/* sxiv: main.c
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

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "sxiv.h"
#include "image.h"
#include "options.h"
#include "window.h"

void on_keypress(XEvent*);
void on_configurenotify(XEvent*);
void on_expose(XEvent*);

static void (*handler[LASTEvent])(XEvent*) = {
	[Expose] = on_expose,
	[ConfigureNotify] = on_configurenotify,
	[KeyPress] = on_keypress
};

img_t img;
win_t win;
unsigned int fileidx;

void run() {
	XEvent ev;

	while (!XNextEvent(win.env.dpy, &ev)) {
		if (handler[ev.type])
			handler[ev.type](&ev);
	}
}

int main(int argc, char **argv) {
	if (parse_options(argc, argv) < 0)
		return 1;

	if (!options->filecnt) {
		print_usage();
		exit(1);
	}

	fileidx = 0;

	img.zoom = 1.0;
	img.scalemode = SCALE_MODE;

	win.w = WIN_WIDTH;
	win.h = WIN_HEIGHT;

	win_open(&win);
	imlib_init(&win);

	img_load(&img, options->filenames[fileidx]);
	img_render(&img, &win);

	run();

	cleanup();

	return 0;
}

void cleanup() {
	static int in = 0;

	if (!in++) {
		imlib_destroy();
		win_close(&win);
	}
}

void on_keypress(XEvent *ev) {
	KeySym keysym;

	if (!ev)
		return;
	
	keysym = XLookupKeysym(&ev->xkey, 0);

	switch (keysym) {
		case XK_Escape:
			cleanup();
			exit(1);
		case XK_q:
			cleanup();
			exit(0);
	}
}

void on_configurenotify(XEvent *ev) {
	if (!ev)
		return;
	
	win_configure(&win, &ev->xconfigure);
}

void on_expose(XEvent *ev) {
}
