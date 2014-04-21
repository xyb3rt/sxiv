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

#define _POSIX_C_SOURCE 200112L
#define _WINDOW_CONFIG

#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>

#include "options.h"
#include "util.h"
#include "window.h"
#include "config.h"
#include "icon/data.h"

enum {
	H_TEXT_PAD = 5,
	V_TEXT_PAD = 1
};

static Cursor carrow;
static Cursor cnone;
static Cursor chand;
static Cursor cwatch;
static GC gc;

static struct {
	int ascent;
	int descent;
	XFontStruct *xfont;
	XFontSet set;
} font;

static int fontheight;
static int barheight;

Atom atoms[ATOM_COUNT];

static Bool fs_support;
static Bool fs_warned;

void win_init_font(Display *dpy, const char *fontstr)
{
	int n;
	char *def, **missing;

	font.set = XCreateFontSet(dpy, fontstr, &missing, &n, &def);
	if (missing)
		XFreeStringList(missing);
	if (font.set) {
		XFontStruct **xfonts;
		char **font_names;

		font.ascent = font.descent = 0;
		XExtentsOfFontSet(font.set);
		n = XFontsOfFontSet(font.set, &xfonts, &font_names);
		while (n--) {
			font.ascent  = MAX(font.ascent, (*xfonts)->ascent);
			font.descent = MAX(font.descent,(*xfonts)->descent);
			xfonts++;
		}
	} else {
		if ((font.xfont = XLoadQueryFont(dpy, fontstr)) == NULL &&
		    (font.xfont = XLoadQueryFont(dpy, "fixed")) == NULL)
		{
			die("could not load font: %s", fontstr);
		}
		font.ascent  = font.xfont->ascent;
		font.descent = font.xfont->descent;
	}
	fontheight = font.ascent + font.descent;
	barheight = fontheight + 2 * V_TEXT_PAD;
}

unsigned long win_alloc_color(win_t *win, const char *name)
{
	XColor col;

	if (win == NULL)
		return 0UL;
	if (XAllocNamedColor(win->env.dpy,
	                     DefaultColormap(win->env.dpy, win->env.scr),
	                     name, &col, &col) == 0)
	{
		die("could not allocate color: %s", name);
	}
	return col.pixel;
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

	if (win == NULL)
		return;

	memset(win, 0, sizeof(win_t));

	e = &win->env;
	if ((e->dpy = XOpenDisplay(NULL)) == NULL)
		die("could not open display");

	e->scr = DefaultScreen(e->dpy);
	e->scrw = DisplayWidth(e->dpy, e->scr);
	e->scrh = DisplayHeight(e->dpy, e->scr);
	e->vis = DefaultVisual(e->dpy, e->scr);
	e->cmap = DefaultColormap(e->dpy, e->scr);
	e->depth = DefaultDepth(e->dpy, e->scr);

	if (setlocale(LC_CTYPE, "") == NULL || XSupportsLocale() == 0)
		warn("no locale support");

	win_init_font(e->dpy, BAR_FONT);

	win->bgcol     = win_alloc_color(win, WIN_BG_COLOR);
	win->fscol     = win_alloc_color(win, WIN_FS_COLOR);
	win->selcol    = win_alloc_color(win, SEL_COLOR);
	win->bar.bgcol = win_alloc_color(win, BAR_BG_COLOR);
	win->bar.fgcol = win_alloc_color(win, BAR_FG_COLOR);
	win->bar.h     = options->hide_bar ? 0 : barheight;

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
	win_env_t *e;
	XClassHint classhint;
	XSetWindowAttributes attr;
	unsigned long attr_mask;
	unsigned long *icon_data;
	XColor col;
	char none_data[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	Pixmap none;
	int gmask;
	XSizeHints sizehints;

	if (win == NULL)
		return;

	e = &win->env;

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

	attr.background_pixel = win->bgcol;
	attr_mask = CWBackPixel;

	win->xwin = XCreateWindow(e->dpy, RootWindow(e->dpy, e->scr),
	                          win->x, win->y, win->w, win->h, 0,
	                          e->depth, InputOutput, e->vis, attr_mask, &attr);
	if (win->xwin == None)
		die("could not create window");
	
	XSelectInput(e->dpy, win->xwin,
	             ExposureMask | ButtonReleaseMask | ButtonPressMask |
	             KeyPressMask | PointerMotionMask | StructureNotifyMask);

	carrow = XCreateFontCursor(e->dpy, XC_left_ptr);
	chand = XCreateFontCursor(e->dpy, XC_fleur);
	cwatch = XCreateFontCursor(e->dpy, XC_watch);

	if (XAllocNamedColor(e->dpy, DefaultColormap(e->dpy, e->scr), "black",
	                     &col, &col) == 0)
	{
		die("could not allocate color: black");
	}
	none = XCreateBitmapFromData(e->dpy, win->xwin, none_data, 8, 8);
	cnone = XCreatePixmapCursor(e->dpy, none, none, &col, &col, 0, 0);

	gc = XCreateGC(e->dpy, win->xwin, 0, None);

	n = icons[ARRLEN(icons)-1].size;
	icon_data = s_malloc((n * n + 2) * sizeof(*icon_data));

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

	XMapWindow(e->dpy, win->xwin);
	XFlush(e->dpy);

	if (options->fullscreen)
		win_toggle_fullscreen(win);
}

void win_close(win_t *win)
{
	if (win == NULL || win->xwin == None)
		return;

	XFreeCursor(win->env.dpy, carrow);
	XFreeCursor(win->env.dpy, cnone);
	XFreeCursor(win->env.dpy, chand);
	XFreeCursor(win->env.dpy, cwatch);

	XFreeGC(win->env.dpy, gc);

	XDestroyWindow(win->env.dpy, win->xwin);
	XCloseDisplay(win->env.dpy);
}

bool win_configure(win_t *win, XConfigureEvent *c)
{
	bool changed;

	if (win == NULL || c == NULL)
		return false;

	if ((changed = win->w != c->width || win->h + win->bar.h != c->height)) {
		if (win->pm != None) {
			XFreePixmap(win->env.dpy, win->pm);
			win->pm = None;
		}
	}

	win->x = c->x;
	win->y = c->y;
	win->w = c->width;
	win->h = c->height - win->bar.h;
	win->bw = c->border_width;

	return changed;
}

void win_expose(win_t *win, XExposeEvent *e)
{
	if (win == NULL || win->xwin == None || win->pm == None || e == NULL)
		return;

	XCopyArea(win->env.dpy, win->pm, win->xwin, gc,
	          e->x, e->y, e->width, e->height, e->x, e->y);
}

void win_toggle_fullscreen(win_t *win)
{
	XEvent ev;
	XClientMessageEvent *cm;

	if (win == NULL || win->xwin == None)
		return;

	if (!fs_support) {
		if (!fs_warned) {
			warn("window manager does not support fullscreen");
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
	if (win == NULL || win->xwin == None)
		return;

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
	int h;
	win_env_t *e;

	if (win == NULL || win->xwin == None)
		return;

	h = win->h + win->bar.h;
	e = &win->env;

	if (win->pm == None)
		win->pm = XCreatePixmap(e->dpy, win->xwin, win->w, h, e->depth);

	XSetForeground(e->dpy, gc, win->fullscreen ? win->fscol : win->bgcol);
	XFillRectangle(e->dpy, win->pm, gc, 0, 0, win->w, h);
}

void win_draw_bar(win_t *win)
{
	int len, olen, x, y, w, tw;
	char rest[3];
	const char *dots = "...";
	win_env_t *e;

	if (win == NULL || win->xwin == None || win->pm == None)
		return;

	e = &win->env;
	y = win->h + font.ascent + V_TEXT_PAD;
	w = win->w;

	XSetForeground(e->dpy, gc, win->bar.bgcol);
	XFillRectangle(e->dpy, win->pm, gc, 0, win->h, win->w, win->bar.h);

	XSetForeground(e->dpy, gc, win->bar.fgcol);
	XSetBackground(e->dpy, gc, win->bar.bgcol);

	if ((len = strlen(win->bar.r)) > 0) {
		if ((tw = win_textwidth(win->bar.r, len, true)) > w)
			return;
		x = win->w - tw + H_TEXT_PAD;
		w -= tw;
		if (font.set)
			XmbDrawString(e->dpy, win->pm, font.set, gc, x, y, win->bar.r, len);
		else
			XDrawString(e->dpy, win->pm, gc, x, y, win->bar.r, len);
	}
	if ((len = strlen(win->bar.l)) > 0) {
		olen = len;
		while (len > 0 && (tw = win_textwidth(win->bar.l, len, true)) > w)
			len--;
		if (len > 0) {
			if (len != olen) {
				w = strlen(dots);
				if (len <= w)
					return;
				memcpy(rest, win->bar.l + len - w, w);
				memcpy(win->bar.l + len - w, dots, w);
			}
			x = H_TEXT_PAD;
			if (font.set)
				XmbDrawString(e->dpy, win->pm, font.set, gc, x, y, win->bar.l, len);
			else
				XDrawString(e->dpy, win->pm, gc, x, y, win->bar.l, len);
			if (len != olen)
			  memcpy(win->bar.l + len - w, rest, w);
		}
	}
}

void win_draw(win_t *win)
{
	if (win == NULL || win->xwin == None || win->pm == None)
		return;

	if (win->bar.h > 0)
		win_draw_bar(win);

	XCopyArea(win->env.dpy, win->pm, win->xwin, gc,
	          0, 0, win->w, win->h + win->bar.h, 0, 0);
}

void win_draw_rect(win_t *win, Pixmap pm, int x, int y, int w, int h,
                   bool fill, int lw, unsigned long col)
{
	XGCValues gcval;

	if (win == NULL || pm == None)
		return;

	gcval.line_width = lw;
	gcval.foreground = col;
	XChangeGC(win->env.dpy, gc, GCForeground | GCLineWidth, &gcval);

	if (fill)
		XFillRectangle(win->env.dpy, pm, gc, x, y, w, h);
	else
		XDrawRectangle(win->env.dpy, pm, gc, x, y, w, h);
}

void win_update_bar(win_t *win)
{
	if (win == NULL || win->xwin == None || win->pm == None)
		return;

	if (win->bar.h > 0) {
		win_draw_bar(win);
		XCopyArea(win->env.dpy, win->pm, win->xwin, gc,
		          0, win->h, win->w, win->bar.h, 0, win->h);
	}
}

int win_textwidth(const char *text, unsigned int len, bool with_padding)
{
	XRectangle r;
	int padding = with_padding ? 2 * H_TEXT_PAD : 0;

	if (font.set) {
		XmbTextExtents(font.set, text, len, NULL, &r);
		return r.width + padding;
	} else {
		return XTextWidth(font.xfont, text, len) + padding;
	}
}

void win_set_title(win_t *win, const char *title)
{
	if (win == NULL || win->xwin == None)
		return;

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
	if (win == NULL || win->xwin == None)
		return;

	switch (cursor) {
		case CURSOR_NONE:
			XDefineCursor(win->env.dpy, win->xwin, cnone);
			break;
		case CURSOR_HAND:
			XDefineCursor(win->env.dpy, win->xwin, chand);
			break;
		case CURSOR_WATCH:
			XDefineCursor(win->env.dpy, win->xwin, cwatch);
			break;
		case CURSOR_ARROW:
		default:
			XDefineCursor(win->env.dpy, win->xwin, carrow);
			break;
	}

	XFlush(win->env.dpy);
}
