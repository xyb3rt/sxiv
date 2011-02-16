/* sxiv: window.c
 * Copyright (c) 2011 Bert Muennich <muennich at informatik.hu-berlin.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <string.h>

#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#include "config.h"
#include "options.h"
#include "util.h"
#include "window.h"

static Cursor arrow;
static Cursor hand;
static GC bgc;

Atom wm_delete_win;

void win_set_sizehints(win_t *win) {
	XSizeHints sizehints;

	if (!win)
		return;

	sizehints.flags = PMinSize | PMaxSize;
	sizehints.min_width = win->w;
	sizehints.max_width = win->w;
	sizehints.min_height = win->h;
	sizehints.max_height = win->h;
	XSetWMNormalHints(win->env.dpy, win->xwin, &sizehints);
}

void win_open(win_t *win) {
	win_env_t *e;
	XClassHint classhint;
	XColor bgcol;
	int gmask;

	if (!win)
		return;

	e = &win->env;
	if (!(e->dpy = XOpenDisplay(NULL)))
		die("could not open display");

	e->scr = DefaultScreen(e->dpy);
	e->scrw = DisplayWidth(e->dpy, e->scr);
	e->scrh = DisplayHeight(e->dpy, e->scr);
	e->vis = DefaultVisual(e->dpy, e->scr);
	e->cmap = DefaultColormap(e->dpy, e->scr);
	e->depth = DefaultDepth(e->dpy, e->scr);

	if (!XAllocNamedColor(e->dpy, DefaultColormap(e->dpy, e->scr), BG_COLOR,
		                    &bgcol, &bgcol))
		die("could not allocate color: %s", BG_COLOR);

	win->bgcol = bgcol.pixel;
	win->pm = 0;

	win->fullscreen = 0;
	
	/* determine window offsets, width & height */
	if (!options->geometry)
		gmask = 0;
	else
		gmask = XParseGeometry(options->geometry, &win->x, &win->y,
		                       &win->w, &win->h);
	if (!(gmask & WidthValue))
		win->w = WIN_WIDTH;
	if (win->w > e->scrw)
		win->w = e->scrw;
	if (!(gmask & HeightValue))
		win->h = WIN_HEIGHT;
	if (win->h > e->scrh)
		win->h = e->scrh;
	if (!(gmask & XValue))
		win->x = (e->scrw - win->w) / 2;
	else if (gmask & XNegative)
		win->x += e->scrw - win->w;
	if (!(gmask & YValue))
		win->y = (e->scrh - win->h) / 2;
	else if (gmask & YNegative)
		win->y += e->scrh - win->h;

	win->xwin = XCreateWindow(e->dpy, RootWindow(e->dpy, e->scr),
	                          win->x, win->y, win->w, win->h, 0,
	                          e->depth, InputOutput, e->vis, 0, None);
	if (win->xwin == None)
		die("could not create window");
	
	XSelectInput(e->dpy, win->xwin, StructureNotifyMask | KeyPressMask |
	             ButtonPressMask | ButtonReleaseMask | Button2MotionMask);

	arrow = XCreateFontCursor(e->dpy, XC_left_ptr);
	hand = XCreateFontCursor(e->dpy, XC_fleur);

	bgc = XCreateGC(e->dpy, win->xwin, 0, None);

	win_set_title(win, "sxiv");

	classhint.res_name = "sxiv";
	classhint.res_class = "sxiv";
	XSetClassHint(e->dpy, win->xwin, &classhint);

	if (options->fixed)
		win_set_sizehints(win);

	XMapWindow(e->dpy, win->xwin);
	XFlush(e->dpy);

	wm_delete_win = XInternAtom(e->dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(e->dpy, win->xwin, &wm_delete_win, 1);
	
	if (options->fullscreen)
		win_toggle_fullscreen(win);
}

void win_close(win_t *win) {
	if (!win)
		return;

	XFreeCursor(win->env.dpy, arrow);
	XFreeCursor(win->env.dpy, hand);

	XFreeGC(win->env.dpy, bgc);

	XDestroyWindow(win->env.dpy, win->xwin);
	XCloseDisplay(win->env.dpy);
}

int win_configure(win_t *win, XConfigureEvent *c) {
	int changed;

	if (!win)
		return 0;
	
	changed = win->w != c->width || win->h != c->height;

	win->x = c->x;
	win->y = c->y;
	win->w = c->width;
	win->h = c->height;
	win->bw = c->border_width;

	return changed;
}

int win_moveresize(win_t *win, int x, int y, unsigned int w, unsigned int h) {
	if (!win)
		return 0;

	x = MAX(0, x);
	y = MAX(0, y);
	w = MIN(w, win->env.scrw - 2 * win->bw);
	h = MIN(h, win->env.scrh - 2 * win->bw);

	if (win->x == x && win->y == y && win->w == w && win->h == h)
		return 0;

	win->x = x;
	win->y = y;
	win->w = w;
	win->h = h;

	if (options->fixed)
		win_set_sizehints(win);

	XMoveResizeWindow(win->env.dpy, win->xwin, win->x, win->y, win->w, win->h);

	return 1;
}

void win_toggle_fullscreen(win_t *win) {
	XEvent ev;
	XClientMessageEvent *cm;

	if (!win)
		return;

	win->fullscreen ^= 1;

	memset(&ev, 0, sizeof(ev));
	ev.type = ClientMessage;

	cm = &ev.xclient;
	cm->window = win->xwin;
	cm->message_type = XInternAtom(win->env.dpy, "_NET_WM_STATE", False);
	cm->format = 32;
	cm->data.l[0] = win->fullscreen;
	cm->data.l[1] = XInternAtom(win->env.dpy, "_NET_WM_STATE_FULLSCREEN", False);
	cm->data.l[2] = XInternAtom(win->env.dpy, "_NET_WM_STATE_ABOVE", False);
	cm->data.l[3] = 0;

	XSendEvent(win->env.dpy, DefaultRootWindow(win->env.dpy), False,
	           SubstructureNotifyMask, &ev);
}

Pixmap win_create_pixmap(win_t *win, int w, int h) {
	if (!win)
		return 0;

	return XCreatePixmap(win->env.dpy, win->xwin, w, h, win->env.depth);
}

void win_free_pixmap(win_t *win, Pixmap pm) {
	if (win && pm)
		XFreePixmap(win->env.dpy, pm);
}

void win_draw_pixmap(win_t *win, Pixmap pm, int x, int y, int w, int h) {
	if (win)
		XCopyArea(win->env.dpy, pm, win->pm, bgc, 0, 0, w, h, x, y);
}

void win_clear(win_t *win) {
	win_env_t *e;
	XGCValues gcval;

	if (!win)
		return;

	e = &win->env;

	if (win->pm)
		XFreePixmap(e->dpy, win->pm);
	win->pm = XCreatePixmap(e->dpy, win->xwin, e->scrw, e->scrh, e->depth);

	gcval.foreground = win->fullscreen ? BlackPixel(e->dpy, e->scr) : win->bgcol;
	XChangeGC(e->dpy, bgc, GCForeground, &gcval);

	XFillRectangle(e->dpy, win->pm, bgc, 0, 0, e->scrw, e->scrh);
}

void win_draw(win_t *win) {
	if (!win)
		return;

	XSetWindowBackgroundPixmap(win->env.dpy, win->xwin, win->pm);
	XClearWindow(win->env.dpy, win->xwin);
}

void win_set_title(win_t *win, const char *title) {
	if (!win)
		return;

	if (!title)
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

void win_set_cursor(win_t *win, win_cur_t cursor) {
	if (!win)
		return;

	switch (cursor) {
		case CURSOR_HAND:
			XDefineCursor(win->env.dpy, win->xwin, hand);
			break;
		case CURSOR_ARROW:
		default:
			XDefineCursor(win->env.dpy, win->xwin, arrow);
			break;
	}
}
