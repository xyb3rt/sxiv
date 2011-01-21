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
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "sxiv.h"
#include "image.h"
#include "options.h"
#include "window.h"

void on_keypress(XEvent*);
void on_configurenotify(XEvent*);

void update_title();

static void (*handler[LASTEvent])(XEvent*) = {
	[KeyPress] = on_keypress,
	[ConfigureNotify] = on_configurenotify
};

img_t img;
win_t win;

const char **filenames;
unsigned int filecnt;
unsigned int fileidx;

#define TITLE_LEN 256
char win_title[TITLE_LEN];

void run() {
	XEvent ev;

	while (!XNextEvent(win.env.dpy, &ev)) {
		if (handler[ev.type])
			handler[ev.type](&ev);
	}
}

int main(int argc, char **argv) {
	int i;

	parse_options(argc, argv);

	if (!options->filecnt) {
		print_usage();
		exit(1);
	}

	if (!(filenames = (const char**) malloc(options->filecnt * sizeof(char*))))
		DIE("could not allocate memory");
	
	fileidx = 0;
	filecnt = 0;

	for (i = 0; i < options->filecnt; ++i) {
		if (!(img_load(&img, options->filenames[i]) < 0))
			filenames[filecnt++] = options->filenames[i];
	}

	if (!filecnt) {
		fprintf(stderr, "sxiv: no valid image filename given, aborting\n");
		exit(1);
	}

	img.zoom = 1.0;
	img.scalemode = SCALE_MODE;

	win.w = WIN_WIDTH;
	win.h = WIN_HEIGHT;

	win_open(&win);
	imlib_init(&win);

	img_load(&img, filenames[fileidx]);
	img_display(&img, &win);
	update_title();

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
	char key;
	int len;
	KeySym keysym;

	if (!ev)
		return;
	
	len = XLookupString(&ev->xkey, &key, 1, &keysym, NULL);

	switch (keysym) {
		case XK_Escape:
			cleanup();
			exit(2);
		case XK_space:
			key = 'n';
			len = 1;
			break;
		case XK_BackSpace:
			key = 'p';
			len = 1;
			break;
	}

	if (!len)
		return;

	switch (key) {
		case 'q':
			cleanup();
			exit(0);
		case 'n':
			if (fileidx + 1 < filecnt) {
				img_load(&img, filenames[++fileidx]);
				img_display(&img, &win);
				update_title();
			}
			break;
		case 'p':
			if (fileidx > 0) {
				img_load(&img, filenames[--fileidx]);
				img_display(&img, &win);
				update_title();
			}
			break;
		case '+':
		case '=':
			if (img_zoom_in(&img)) {
				img_render(&img, &win);
				update_title();
			}
			break;
		case '-':
			if (img_zoom_out(&img)) {
				img_render(&img, &win);
				update_title();
			}
			break;
	}
}

void on_configurenotify(XEvent *ev) {
	if (!ev)
		return;
	
	win_configure(&win, &ev->xconfigure);
}

void update_title() {
	int n;

	n = snprintf(win_title, TITLE_LEN, "sxiv: [%d/%d] <%d%%> %s", fileidx + 1,
	             filecnt, (int) (img.zoom * 100.0), filenames[fileidx]);
	
	if (n >= TITLE_LEN) {
		win_title[TITLE_LEN - 2] = '.';
		win_title[TITLE_LEN - 3] = '.';
		win_title[TITLE_LEN - 4] = '.';
	}

	win_set_title(&win, win_title);
}
