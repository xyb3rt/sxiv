/* Copyright 2011-2013 Bert Muennich
 *
 * This file is part of sxiv.
 *
 * sxiv is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * sxiv is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with sxiv.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sxiv.h"
#define _WINDOW_CONFIG
#include "config.h"
#include "icon/data.h"

#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>

enum {
	H_TEXT_PAD = 5,
	V_TEXT_PAD = 1
};

static struct {
	int name;
	Cursor icon;
} cursors[CURSOR_COUNT] = {
	{ XC_left_ptr }, { XC_dotbox }, { XC_watch },
	{ XC_sb_left_arrow }, { XC_sb_right_arrow }
};

static GC gc;

static XftFont *font;
static int fontheight;
static int barheight;

Atom atoms[ATOM_COUNT];

static Bool fs_support;
static Bool fs_warned;

void win_init_font(const win_env_t *e, const char *fontstr)
{
	if ((font = XftFontOpenName(e->dpy, e->scr, fontstr)) == NULL)
		error(EXIT_FAILURE, 0, "Error loading font '%s'", fontstr);
	fontheight = font->ascent + font->descent;
	barheight = fontheight + 2 * V_TEXT_PAD;
}

void win_alloc_color(const win_env_t *e, const char *name, XftColor *col)
{
	if (!XftColorAllocName(e->dpy, DefaultVisual(e->dpy, e->scr),
	                       DefaultColormap(e->dpy, e->scr), name, col))
	{
		error(EXIT_FAILURE, 0, "Error allocating color '%s'", name);
	}
}

void win_check_wm_support(Display *dpy, Window root)
{
	int format;
	long offset = 0, length = 16;
	Atom *data, type;
	unsigned long i, nitems, bytes_left;
	Bool found = False;

	while (!found && length > 0) {
		if (XGetWindowProperty(dpy, root, atoms[ATOM__NET_SUPPORTED],
		                       offset, length, False, XA_ATOM, &type, &format,
		                       &nitems, &bytes_left, (unsigned char**) &data))
		{
			break;
		}
		if (type == XA_ATOM && format == 32) {
			for (i = 0; i < nitems; i++) {
				if (data[i] == atoms[ATOM__NET_WM_STATE_FULLSCREEN]) {
					found = True;
					fs_support = True;
					break;
				}
			}
		}
		XFree(data);
		offset += nitems;
		length = MIN(length, bytes_left / 4);
	}
}

#define INIT_ATOM_(atom) \
	atoms[ATOM_##atom] = XInternAtom(e->dpy, #atom, False);

void win_init(win_t *win)
{
	win_env_t *e;

	memset(win, 0, sizeof(win_t));

	e = &win->env;
	if ((e->dpy = XOpenDisplay(NULL)) == NULL)
		error(EXIT_FAILURE, 0, "Error opening X display");

	e->scr = DefaultScreen(e->dpy);
	e->scrw = DisplayWidth(e->dpy, e->scr);
	e->scrh = DisplayHeight(e->dpy, e->scr);
	e->vis = DefaultVisual(e->dpy, e->scr);
	e->cmap = DefaultColormap(e->dpy, e->scr);
	e->depth = DefaultDepth(e->dpy, e->scr);

	if (setlocale(LC_CTYPE, "") == NULL || XSupportsLocale() == 0)
		error(0, 0, "No locale support");

	win_init_font(e, BAR_FONT);

	win_alloc_color(e, WIN_BG_COLOR, &win->bgcol);
	win_alloc_color(e, WIN_FS_COLOR, &win->fscol);
	win_alloc_color(e, SEL_COLOR,    &win->selcol);
	win_alloc_color(e, BAR_BG_COLOR, &win->bar.bgcol);
	win_alloc_color(e, BAR_FG_COLOR, &win->bar.fgcol);

	win->bar.l.size = BAR_L_LEN;
	win->bar.r.size = BAR_R_LEN;
	win->bar.l.buf = emalloc(win->bar.l.size);
	win->bar.r.buf = emalloc(win->bar.r.size);
	win->bar.h = options->hide_bar ? 0 : barheight;

	INIT_ATOM_(WM_DELETE_WINDOW);
	INIT_ATOM_(_NET_WM_NAME);
	INIT_ATOM_(_NET_WM_ICON_NAME);
	INIT_ATOM_(_NET_WM_ICON);
	INIT_ATOM_(_NET_WM_STATE);
	INIT_ATOM_(_NET_WM_STATE_FULLSCREEN);
	INIT_ATOM_(_NET_SUPPORTED);

	win_check_wm_support(e->dpy, RootWindow(e->dpy, e->scr));
}

void win_open(win_t *win)
{
	int c, i, j, n;
	long parent;
	win_env_t *e;
	XClassHint classhint;
	unsigned long *icon_data;
	XColor col;
	Cursor *cnone = &cursors[CURSOR_NONE].icon;
	char none_data[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	Pixmap none;
	int gmask;
	XSizeHints sizehints;
	Bool fullscreen = options->fullscreen && fs_support;

	e = &win->env;
	parent = options->embed != 0 ? options->embed : RootWindow(e->dpy, e->scr);

	sizehints.flags = PWinGravity;
	sizehints.win_gravity = NorthWestGravity;

	/* determine window offsets, width & height */
	if (options->geometry == NULL)
		gmask = 0;
	else
		gmask = XParseGeometry(options->geometry, &win->x, &win->y,
		                       &win->w, &win->h);
	if ((gmask & WidthValue) != 0)
		sizehints.flags |= USSize;
	else
		win->w = WIN_WIDTH;
	if ((gmask & HeightValue) != 0)
		sizehints.flags |= USSize;
	else
		win->h = WIN_HEIGHT;
	if ((gmask & XValue) != 0) {
		if ((gmask & XNegative) != 0) {
			win->x += e->scrw - win->w;
			sizehints.win_gravity = NorthEastGravity;
		}
		sizehints.flags |= USPosition;
	} else {
		win->x = 0;
	}
	if ((gmask & YValue) != 0) {
		if ((gmask & YNegative) != 0) {
			win->y += e->scrh - win->h;
			sizehints.win_gravity = sizehints.win_gravity == NorthEastGravity
			                      ? SouthEastGravity : SouthWestGravity;
		}
		sizehints.flags |= USPosition;
	} else {
		win->y = 0;
	}

	win->xwin = XCreateWindow(e->dpy, parent,
	                          win->x, win->y, win->w, win->h, 0,
	                          e->depth, InputOutput, e->vis, 0, NULL);
	if (win->xwin == None)
		error(EXIT_FAILURE, 0, "Error creating X window");
	
	XSelectInput(e->dpy, win->xwin,
	             ButtonReleaseMask | ButtonPressMask | KeyPressMask |
	             PointerMotionMask | StructureNotifyMask);

	for (i = 0; i < ARRLEN(cursors); i++) {
		if (i != CURSOR_NONE)
			cursors[i].icon = XCreateFontCursor(e->dpy, cursors[i].name);
	}
	if (XAllocNamedColor(e->dpy, DefaultColormap(e->dpy, e->scr), "black",
	                     &col, &col) == 0)
	{
		error(EXIT_FAILURE, 0, "Error allocating color 'black'");
	}
	none = XCreateBitmapFromData(e->dpy, win->xwin, none_data, 8, 8);
	*cnone = XCreatePixmapCursor(e->dpy, none, none, &col, &col, 0, 0);

	gc = XCreateGC(e->dpy, win->xwin, 0, None);

	n = icons[ARRLEN(icons)-1].size;
	icon_data = emalloc((n * n + 2) * sizeof(*icon_data));

	for (i = 0; i < ARRLEN(icons); i++) {
		n = 0;
		icon_data[n++] = icons[i].size;
		icon_data[n++] = icons[i].size;

		for (j = 0; j < icons[i].cnt; j++) {
			for (c = icons[i].data[j] >> 4; c >= 0; c--)
				icon_data[n++] = icon_colors[icons[i].data[j] & 0x0F];
		}
		XChangeProperty(e->dpy, win->xwin,
		                atoms[ATOM__NET_WM_ICON], XA_CARDINAL, 32,
		                i == 0 ? PropModeReplace : PropModeAppend,
		                (unsigned char *) icon_data, n);
	}
	free(icon_data);

	win_set_title(win, "sxiv");

	classhint.res_class = "Sxiv";
	classhint.res_name = options->res_name != NULL ? options->res_name : "sxiv";
	XSetClassHint(e->dpy, win->xwin, &classhint);

	XSetWMProtocols(e->dpy, win->xwin, &atoms[ATOM_WM_DELETE_WINDOW], 1);

	sizehints.width = win->w;
	sizehints.height = win->h;
	sizehints.x = win->x;
	sizehints.y = win->y;
	XSetWMNormalHints(win->env.dpy, win->xwin, &sizehints);

	win->h -= win->bar.h;

	win->buf.w = e->scrw;
	win->buf.h = e->scrh;
	win->buf.pm = XCreatePixmap(e->dpy, win->xwin,
	                            win->buf.w, win->buf.h, e->depth);
	XSetForeground(e->dpy, gc, fullscreen ? win->fscol.pixel : win->bgcol.pixel);
	XFillRectangle(e->dpy, win->buf.pm, gc, 0, 0, win->buf.w, win->buf.h);
	XSetWindowBackgroundPixmap(e->dpy, win->xwin, win->buf.pm);

	XMapWindow(e->dpy, win->xwin);
	XFlush(e->dpy);

	if (fullscreen)
		win_toggle_fullscreen(win);
}

CLEANUP void win_close(win_t *win)
{
	int i;

	for (i = 0; i < ARRLEN(cursors); i++)
		XFreeCursor(win->env.dpy, cursors[i].icon);

	XFreeGC(win->env.dpy, gc);

	XDestroyWindow(win->env.dpy, win->xwin);
	XCloseDisplay(win->env.dpy);
}

bool win_configure(win_t *win, XConfigureEvent *c)
{
	bool changed;

	changed = win->w != c->width || win->h + win->bar.h != c->height;

	win->x = c->x;
	win->y = c->y;
	win->w = c->width;
	win->h = c->height - win->bar.h;
	win->bw = c->border_width;

	return changed;
}

void win_toggle_fullscreen(win_t *win)
{
	XEvent ev;
	XClientMessageEvent *cm;

	if (!fs_support) {
		if (!fs_warned) {
			error(0, 0, "No fullscreen support");
			fs_warned = True;
		}
		return;
	}
	win->fullscreen = !win->fullscreen;

	memset(&ev, 0, sizeof(ev));
	ev.type = ClientMessage;

	cm = &ev.xclient;
	cm->window = win->xwin;
	cm->message_type = atoms[ATOM__NET_WM_STATE];
	cm->format = 32;
	cm->data.l[0] = win->fullscreen;
	cm->data.l[1] = atoms[ATOM__NET_WM_STATE_FULLSCREEN];
	cm->data.l[2] = cm->data.l[3] = 0;

	XSendEvent(win->env.dpy, DefaultRootWindow(win->env.dpy), False,
	           SubstructureNotifyMask | SubstructureRedirectMask, &ev);
}

void win_toggle_bar(win_t *win)
{
	if (win->bar.h != 0) {
		win->h += win->bar.h;
		win->bar.h = 0;
	} else {
		win->bar.h = barheight;
		win->h -= win->bar.h;
	}
}

void win_clear(win_t *win)
{
	win_env_t *e;

	e = &win->env;

	if (win->w > win->buf.w || win->h + win->bar.h > win->buf.h) {
		XFreePixmap(e->dpy, win->buf.pm);
		win->buf.w = MAX(win->buf.w, win->w);
		win->buf.h = MAX(win->buf.h, win->h + win->bar.h);
		win->buf.pm = XCreatePixmap(e->dpy, win->xwin,
		                            win->buf.w, win->buf.h, e->depth);
	}
	XSetForeground(e->dpy, gc, win->fullscreen ? win->fscol.pixel : win->bgcol.pixel);
	XFillRectangle(e->dpy, win->buf.pm, gc, 0, 0, win->buf.w, win->buf.h);
}

void win_draw_bar(win_t *win)
{
	int len, olen, x, y, w, tw;
	char rest[3];
	const char *dots = "...";
	win_env_t *e;
	win_bar_t *l, *r;
	XftDraw *d;

	if ((l = &win->bar.l)->buf == NULL || (r = &win->bar.r)->buf == NULL)
		return;

	e = &win->env;
	y = win->h + font->ascent + V_TEXT_PAD;
	w = win->w;
	d = XftDrawCreate(e->dpy, win->buf.pm, DefaultVisual(e->dpy, e->scr),
	                  DefaultColormap(e->dpy, e->scr));

	XSetForeground(e->dpy, gc, win->bar.bgcol.pixel);
	XFillRectangle(e->dpy, win->buf.pm, gc, 0, win->h, win->w, win->bar.h);

	XSetForeground(e->dpy, gc, win->bar.fgcol.pixel);
	XSetBackground(e->dpy, gc, win->bar.bgcol.pixel);

	if ((len = strlen(r->buf)) > 0) {
		if ((tw = win_textwidth(e, r->buf, len, true)) > w)
			return;
		x = win->w - tw + H_TEXT_PAD;
		w -= tw;
		XftDrawStringUtf8(d, &win->bar.fgcol, font, x, y, (XftChar8*)r->buf, len);
	}
	if ((len = strlen(l->buf)) > 0) {
		olen = len;
		while (len > 0 && (tw = win_textwidth(e, l->buf, len, true)) > w)
			len--;
		if (len > 0) {
			if (len != olen) {
				w = strlen(dots);
				if (len <= w)
					return;
				memcpy(rest, l->buf + len - w, w);
				memcpy(l->buf + len - w, dots, w);
			}
			x = H_TEXT_PAD;
			XftDrawStringUtf8(d, &win->bar.fgcol, font, x, y, (XftChar8*)l->buf, len);
			if (len != olen)
			  memcpy(l->buf + len - w, rest, w);
		}
	}
	XftDrawDestroy(d);
}

void win_draw(win_t *win)
{
	if (win->bar.h > 0)
		win_draw_bar(win);

	XSetWindowBackgroundPixmap(win->env.dpy, win->xwin, win->buf.pm);
	XClearWindow(win->env.dpy, win->xwin);
	XFlush(win->env.dpy);
}

void win_draw_rect(win_t *win, int x, int y, int w, int h, bool fill, int lw,
                   unsigned long col)
{
	XGCValues gcval;

	gcval.line_width = lw;
	gcval.foreground = col;
	XChangeGC(win->env.dpy, gc, GCForeground | GCLineWidth, &gcval);

	if (fill)
		XFillRectangle(win->env.dpy, win->buf.pm, gc, x, y, w, h);
	else
		XDrawRectangle(win->env.dpy, win->buf.pm, gc, x, y, w, h);
}

int win_textwidth(const win_env_t *e, const char *text, unsigned int len, bool with_padding)
{
	XGlyphInfo ext;

	XftTextExtentsUtf8(e->dpy, font, (XftChar8*)text, len, &ext);
	return ext.xOff + (with_padding ? 2 * H_TEXT_PAD : 0);
}

void win_set_title(win_t *win, const char *title)
{
	if (title == NULL)
		title = "sxiv";

	XStoreName(win->env.dpy, win->xwin, title);
	XSetIconName(win->env.dpy, win->xwin, title);

	XChangeProperty(win->env.dpy, win->xwin, atoms[ATOM__NET_WM_NAME],
	                XInternAtom(win->env.dpy, "UTF8_STRING", False), 8,
	                PropModeReplace, (unsigned char *) title, strlen(title));
	XChangeProperty(win->env.dpy, win->xwin, atoms[ATOM__NET_WM_ICON_NAME],
	                XInternAtom(win->env.dpy, "UTF8_STRING", False), 8,
	                PropModeReplace, (unsigned char *) title, strlen(title));
}

void win_set_cursor(win_t *win, cursor_t cursor)
{
	if (cursor >= 0 && cursor < ARRLEN(cursors)) {
		XDefineCursor(win->env.dpy, win->xwin, cursors[cursor].icon);
		XFlush(win->env.dpy);
	}
}

void win_cursor_pos(win_t *win, int *x, int *y)
{
	int i;
	unsigned int ui;
	Window w;

	if (!XQueryPointer(win->env.dpy, win->xwin, &w, &w, &i, &i, x, y, &ui))
		*x = *y = 0;
}

