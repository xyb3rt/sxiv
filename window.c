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

Display *dpy;
int scr;
int scrw, scrh;
GC gc;
XColor bgcol;

void win_open(win_t *win) {
	XClassHint *classhint;

	if (win == NULL)
		return;

	if (!(dpy = XOpenDisplay(NULL)))
		FATAL("could not open display");
	
	scr = DefaultScreen(dpy);
	scrw = DisplayWidth(dpy, scr);
	scrh = DisplayHeight(dpy, scr);

	if (!XAllocNamedColor(dpy, DefaultColormap(dpy, scr), BG_COLOR,
			&bgcol, &bgcol))
		FATAL("could not allocate color: %s", BG_COLOR);

	if (win->w > scrw)
		win->w = scrw;
	if (win->h > scrh)
		win->h = scrh;
	win->x = (scrw - win->w) / 2;
	win->y = (scrh - win->h) / 2;

	win->xwin = XCreateWindow(dpy, RootWindow(dpy, scr),
			win->x, win->y, win->w, win->h, 0, DefaultDepth(dpy, scr), InputOutput,
			DefaultVisual(dpy, scr), 0, NULL);
	if (win->xwin == None)
		FATAL("could not create window");
	
	XSelectInput(dpy, win->xwin,
			StructureNotifyMask | ExposureMask | KeyPressMask);

	gc = XCreateGC(dpy, win->xwin, 0, NULL);

	if ((classhint = XAllocClassHint())) {
		classhint->res_name = "sxvi";
		classhint->res_class = "sxvi";
		XSetClassHint(dpy, win->xwin, classhint);
		XFree(classhint);
	}

	XMapWindow(dpy, win->xwin);
	XFlush(dpy);
}

void win_close(win_t *win) {
	if (win == NULL)
		return;

	XDestroyWindow(dpy, win->xwin);
	XFreeGC(dpy, gc);
	XCloseDisplay(dpy);
}
