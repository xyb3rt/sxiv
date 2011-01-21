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

#include <stdlib.h>
#include <stdio.h>

#include <X11/Xutil.h>

#include "sxiv.h"
#include "window.h"

void win_open(win_t *win) {
	win_env_t *e;
	XClassHint *classhint;
	XColor bgcol;
	XGCValues gcval;

	if (!win)
		return;

	e = &win->env;
	if (!(e->dpy = XOpenDisplay(NULL)))
		DIE("could not open display");

	e->scr = DefaultScreen(e->dpy);
	e->scrw = DisplayWidth(e->dpy, e->scr);
	e->scrh = DisplayHeight(e->dpy, e->scr);
	e->vis = DefaultVisual(e->dpy, e->scr);
	e->cmap = DefaultColormap(e->dpy, e->scr);
	e->depth = DefaultDepth(e->dpy, e->scr);

	if (!XAllocNamedColor(e->dpy, DefaultColormap(e->dpy, e->scr), BG_COLOR,
		                    &bgcol, &bgcol))
		DIE("could not allocate color: %s", BG_COLOR);

	win->w = WIN_WIDTH;
	win->h = WIN_HEIGHT;
	if (win->w > e->scrw)
		win->w = e->scrw;
	if (win->h > e->scrh)
		win->h = e->scrh;
	win->x = (e->scrw - win->w) / 2;
	win->y = (e->scrh - win->h) / 2;

	win->xwin = XCreateWindow(e->dpy, RootWindow(e->dpy, e->scr),
	                          win->x, win->y, win->w, win->h, 0,
	                          e->depth, InputOutput, e->vis, 0, None);
	if (win->xwin == None)
		DIE("could not create window");
	
	XSelectInput(e->dpy, win->xwin,
	             StructureNotifyMask | KeyPressMask);

	gcval.foreground = bgcol.pixel;
	win->bgc = XCreateGC(e->dpy, win->xwin, GCForeground, &gcval);
	win->pm = 0;

	win_set_title(win, "sxiv");

	if ((classhint = XAllocClassHint())) {
		classhint->res_name = "sxiv";
		classhint->res_class = "sxiv";
		XSetClassHint(e->dpy, win->xwin, classhint);
		XFree(classhint);
	}

	XMapWindow(e->dpy, win->xwin);
	XFlush(e->dpy);
}

void win_close(win_t *win) {
	if (!win)
		return;

	XDestroyWindow(win->env.dpy, win->xwin);
	XCloseDisplay(win->env.dpy);
}

void win_set_title(win_t *win, const char *title) {
	if (!win)
		return;

	if (!title)
		title = "sxiv";

	XStoreName(win->env.dpy, win->xwin, title);
	XSetIconName(win->env.dpy, win->xwin, title);
}

int win_configure(win_t *win, XConfigureEvent *cev) {
	int changed;

	if (!win)
		return 0;
	
	changed = win->x != cev->x || win->y != cev->y ||
	          win->w != cev->width || win->h != cev->height;
	win->x = cev->x;
	win->y = cev->y;
	win->w = cev->width;
	win->h = cev->height;
	win->bw = cev->border_width;
	return changed;
}

void win_clear(win_t *win) {
	win_env_t *e;

	if (!win)
		return;

	e = &win->env;

	if (win->pm)
		XFreePixmap(e->dpy, win->pm);
	win->pm = XCreatePixmap(e->dpy, win->xwin, e->scrw, e->scrh, e->depth);
	XFillRectangle(e->dpy, win->pm, win->bgc, 0, 0, e->scrw, e->scrh);
}

void win_draw(win_t *win) {
	if (!win)
		return;

	XSetWindowBackgroundPixmap(win->env.dpy, win->xwin, win->pm);
	XClearWindow(win->env.dpy, win->xwin);
}
