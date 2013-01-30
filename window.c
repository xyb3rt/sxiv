/* sxiv: window.c
 * Copyright (c) 2012 Bert Muennich <be.muennich at googlemail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#define _POSIX_C_SOURCE 200112L
#define _WINDOW_CONFIG

#include <string.h>
#include <locale.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#include "options.h"
#include "util.h"
#include "window.h"
#include "config.h"

enum {
	H_TEXT_PAD = 5,
	V_TEXT_PAD = 1
};

static Cursor carrow;
static Cursor cnone;
static Cursor chand;
static Cursor cwatch;
static GC gc;

Atom wm_delete_win;

static struct {
	int ascent;
	int descent;
	XFontStruct *xfont;
	XFontSet set;
} font;

static int fontheight;
static int barheight;

void win_init_font(Display *dpy, const char *fontstr) {
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

unsigned long win_alloc_color(win_t *win, const char *name) {
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

void win_init(win_t *win) {
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

	win->white     = WhitePixel(e->dpy, e->scr);
	win->bgcol     = win_alloc_color(win, WIN_BG_COLOR);
	win->fscol     = win_alloc_color(win, WIN_FS_COLOR);
	win->selcol    = win_alloc_color(win, SEL_COLOR);
	win->bar.bgcol = win_alloc_color(win, BAR_BG_COLOR);
	win->bar.fgcol = win_alloc_color(win, BAR_FG_COLOR);

	win->sizehints.flags = PWinGravity;
	win->sizehints.win_gravity = NorthWestGravity;

	if (setlocale(LC_CTYPE, "") == NULL || XSupportsLocale() == 0)
		warn("no locale support");

	win_init_font(e->dpy, BAR_FONT);

	wm_delete_win = XInternAtom(e->dpy, "WM_DELETE_WINDOW", False);
}

void win_set_sizehints(win_t *win) {

	if (win == NULL || win->xwin == None)
		return;

	if ((win->sizehints.flags & PMinSize) == 1) {
		win->sizehints.min_width = win->w;
		win->sizehints.min_height = win->h + win->bar.h;
	}
	if ((win->sizehints.flags & PMaxSize) == 1) {
		win->sizehints.max_width = win->w;
		win->sizehints.max_height = win->h + win->bar.h;
	}
	if ((win->sizehints.flags & USPosition) == 1) {
		win->sizehints.x = win->x;
		win->sizehints.y = win->y;
	}

	XSetWMNormalHints(win->env.dpy, win->xwin, &win->sizehints);
}

void win_open(win_t *win) {
	win_env_t *e;
	XClassHint classhint;
	XColor col;
	char none_data[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	Pixmap none;
	int gmask;

	if (win == NULL)
		return;

	e = &win->env;

	/* determine window offsets, width & height */
	if (options->geometry == NULL)
		gmask = 0;
	else
		gmask = XParseGeometry(options->geometry, &win->x, &win->y,
		                       &win->w, &win->h);
	if ((gmask & WidthValue) == 0) {
		win->w = WIN_WIDTH;
	} else {
		win->sizehints.flags |= USSize;
	}
	if (win->w > e->scrw)
		win->w = e->scrw;
	if ((gmask & HeightValue) == 0) {
		win->h = WIN_HEIGHT;
	} else {
		win->sizehints.flags |= USSize;
	}
	if (win->h > e->scrh)
		win->h = e->scrh;
	if ((gmask & XValue) == 0) {
		win->x = (e->scrw - win->w) / 2;
	} else {
		if ((gmask & XNegative) != 0) {
			win->x += e->scrw - win->w;
			win->sizehints.win_gravity = NorthEastGravity;
		}
		win->sizehints.flags |= USPosition;
	}
	if ((gmask & YValue) == 0) {
		win->y = (e->scrh - win->h) / 2;
	} else {
		if ((gmask & YNegative) != 0) {
			win->y += e->scrh - win->h;
			if (win->sizehints.win_gravity == NorthEastGravity) {
				win->sizehints.win_gravity = SouthEastGravity;
			} else {
				win->sizehints.win_gravity = SouthWestGravity;
			}
		}
		win->sizehints.flags |= USPosition;
	}

	win->xwin = XCreateWindow(e->dpy, RootWindow(e->dpy, e->scr),
	                          win->x, win->y, win->w, win->h, 0,
	                          e->depth, InputOutput, e->vis, 0, None);
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

	win_set_title(win, "sxiv");

	classhint.res_class = "Sxiv";
	classhint.res_name = options->res_name != NULL ? options->res_name : "sxiv";
	XSetClassHint(e->dpy, win->xwin, &classhint);

	XSetWMProtocols(e->dpy, win->xwin, &wm_delete_win, 1);

	if (!options->hide_bar) {
		win->bar.h = barheight;
		win->h -= win->bar.h;
	}

	if (options->fixed_win)
		win->sizehints.flags |= PMinSize | PMaxSize;

	win_set_sizehints(win);

	XMapWindow(e->dpy, win->xwin);
	XFlush(e->dpy);

	if (options->fullscreen)
		win_toggle_fullscreen(win);
}

void win_close(win_t *win) {
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

bool win_configure(win_t *win, XConfigureEvent *c) {
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

void win_expose(win_t *win, XExposeEvent *e) {
	if (win == NULL || win->xwin == None || win->pm == None || e == NULL)
		return;

	XCopyArea(win->env.dpy, win->pm, win->xwin, gc,
	          e->x, e->y, e->width, e->height, e->x, e->y);
}

bool win_moveresize(win_t *win, int x, int y, unsigned int w, unsigned int h) {
	if (win == NULL || win->xwin == None)
		return false;

	x = MAX(0, x);
	y = MAX(0, y);
	w = MIN(w, win->env.scrw - 2 * win->bw);
	h = MIN(h, win->env.scrh - 2 * win->bw);

	if (win->x == x && win->y == y && win->w == w && win->h + win->bar.h == h)
		return false;

	win->x = x;
	win->y = y;
	win->w = w;
	win->h = h - win->bar.h;


	win_set_sizehints(win);

	XMoveResizeWindow(win->env.dpy, win->xwin, x, y, w, h);

	return true;
}

void win_toggle_fullscreen(win_t *win) {
	XEvent ev;
	XClientMessageEvent *cm;

	if (win == NULL || win->xwin == None)
		return;

	win->fullscreen = !win->fullscreen;

	memset(&ev, 0, sizeof(ev));
	ev.type = ClientMessage;

	cm = &ev.xclient;
	cm->window = win->xwin;
	cm->message_type = XInternAtom(win->env.dpy, "_NET_WM_STATE", False);
	cm->format = 32;
	cm->data.l[0] = win->fullscreen;
	cm->data.l[1] = XInternAtom(win->env.dpy, "_NET_WM_STATE_FULLSCREEN", False);
	cm->data.l[2] = cm->data.l[3] = 0;

	XSendEvent(win->env.dpy, DefaultRootWindow(win->env.dpy), False,
	           SubstructureNotifyMask | SubstructureRedirectMask, &ev);
}

void win_toggle_bar(win_t *win) {
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

void win_clear(win_t *win) {
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

void win_draw_bar(win_t *win) {
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

	if (win->bar.r != NULL) {
		len = strlen(win->bar.r);
		if (len > 0) {
			if ((tw = win_textwidth(win->bar.r, len, true)) > w)
				return;
			x = win->w - tw + H_TEXT_PAD;
			w -= tw;
			if (font.set)
				XmbDrawString(e->dpy, win->pm, font.set, gc, x, y, win->bar.r, len);
			else
				XDrawString(e->dpy, win->pm, gc, x, y, win->bar.r, len);
		}
	}
	if (win->bar.l != NULL) {
		olen = len = strlen(win->bar.l);
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

void win_draw(win_t *win) {
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

int win_textwidth(const char *text, unsigned int len, bool with_padding) {
	XRectangle r;
	int padding = with_padding ? 2 * H_TEXT_PAD : 0;

	if (font.set) {
		XmbTextExtents(font.set, text, len, NULL, &r);
		return r.width + padding;
	} else {
		return XTextWidth(font.xfont, text, len) + padding;
	}
}

void win_set_title(win_t *win, const char *title) {
	if (win == NULL || win->xwin == None)
		return;

	if (title == NULL)
		title = "sxiv";

	XStoreName(win->env.dpy, win->xwin, title);
	XSetIconName(win->env.dpy, win->xwin, title);

	XChangeProperty(win->env.dpy, win->xwin,
	                XInternAtom(win->env.dpy, "_NET_WM_NAME", False),
	                XInternAtom(win->env.dpy, "UTF8_STRING", False), 8,
	                PropModeReplace, (unsigned char *) title, strlen(title));
	XChangeProperty(win->env.dpy, win->xwin,
	                XInternAtom(win->env.dpy, "_NET_WM_ICON_NAME", False),
	                XInternAtom(win->env.dpy, "UTF8_STRING", False), 8,
	                PropModeReplace, (unsigned char *) title, strlen(title));
}

void win_set_bar_info(win_t *win, char *linfo, char *rinfo) {
	if (win != NULL) {
		win->bar.l = linfo;
		win->bar.r = rinfo;
	}
}

void win_set_cursor(win_t *win, cursor_t cursor) {
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
