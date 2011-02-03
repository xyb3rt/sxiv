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
#include <string.h>
#include <dirent.h>
#include <sys/select.h>
#include <sys/stat.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "image.h"
#include "options.h"
#include "util.h"
#include "window.h"

void on_keypress(XEvent*);
void on_buttonpress(XEvent*);
void on_buttonrelease(XEvent*);
void on_motionnotify(XEvent*);
void on_configurenotify(XEvent*);

void update_title();
void check_append(const char*);
void read_dir_rec(const char*);

static void (*handler[LASTEvent])(XEvent*);

img_t img;
win_t win;

#define DNAME_CNT 512
#define FNAME_CNT 4096
const char **filenames;
int filecnt, fileidx;

unsigned char timeout;

int mox;
int moy;

#define TITLE_LEN 256
char win_title[TITLE_LEN];

void cleanup() {
	static int in = 0;

	if (!in++) {
		img_free(&img);
		win_close(&win);
	}
}

void run() {
	int xfd;
	fd_set fds;
	struct timeval t;
	XEvent ev;

	handler[KeyPress] = on_keypress;
	handler[ButtonPress] = on_buttonpress;
	handler[ButtonRelease] = on_buttonrelease;
	handler[MotionNotify] = on_motionnotify;
	handler[ConfigureNotify] = on_configurenotify;

	timeout = 0;

	while (1) {
		if (timeout) {
			t.tv_sec = 0;
			t.tv_usec = 250;
			xfd = ConnectionNumber(win.env.dpy);
			FD_ZERO(&fds);
			FD_SET(xfd, &fds);
			
			if (!XPending(win.env.dpy) && !select(xfd + 1, &fds, 0, 0, &t)) {
				img_render(&img, &win);
				timeout = 0;
			}
		}

		if (!XNextEvent(win.env.dpy, &ev) && handler[ev.type])
			handler[ev.type](&ev);
	}
}

int main(int argc, char **argv) {
	int i;
	const char *filename;
	struct stat fstats;

	parse_options(argc, argv);

	if (!options->filecnt) {
		print_usage();
		exit(1);
	}

	if (options->recursive)
		filecnt = FNAME_CNT;
	else
		filecnt = options->filecnt;

	filenames = (const char**) s_malloc(filecnt * sizeof(const char*));
	fileidx = 0;

	for (i = 0; i < options->filecnt; ++i) {
		filename = options->filenames[i];
		if (stat(filename, &fstats)) {
			warn("could not stat file: %s", filename);
		} else if (S_ISDIR(fstats.st_mode)) {
			if (options->recursive)
				read_dir_rec(filename);
			else
				warn("ignoring directory: %s", filename);
		} else {
			check_append(filename);
		}
	}

	filecnt = fileidx;
	fileidx = 0;

	if (!filecnt) {
		fprintf(stderr, "sxiv: no valid image filename given, aborting\n");
		exit(1);
	}

	win_open(&win);
	img_init(&img, &win);

	img_load(&img, filenames[fileidx]);
	img_render(&img, &win);
	update_title();

	run();

	cleanup();

	return 0;
}

void on_keypress(XEvent *ev) {
	char key;
	KeySym ksym;
	int changed;

	if (!ev)
		return;
	
	XLookupString(&ev->xkey, &key, 1, &ksym, NULL);
	changed = 0;

	switch (ksym) {
		case XK_Escape:
			cleanup();
			exit(2);
		case XK_q:
			cleanup();
			exit(0);

		/* navigate image list */
		case XK_n:
		case XK_space:
			if (fileidx + 1 < filecnt) {
				changed = img_load(&img, filenames[++fileidx]);
			}
			break;
		case XK_p:
		case XK_BackSpace:
			if (fileidx > 0) {
				changed = img_load(&img, filenames[--fileidx]);
			}
			break;
		case XK_bracketleft:
			if (fileidx != 0) {
				fileidx = MAX(0, fileidx - 10);
				changed = img_load(&img, filenames[fileidx]);
			}
			break;
		case XK_bracketright:
			if (fileidx != filecnt - 1) {
				fileidx = MIN(fileidx + 10, filecnt - 1);
				changed = img_load(&img, filenames[fileidx]);
			}
			break;
		case XK_g:
			if (fileidx != 0) {
				fileidx = 0;
				changed = img_load(&img, filenames[fileidx]);
			}
			break;
		case XK_G:
			if (fileidx != filecnt - 1) {
				fileidx = filecnt - 1;
				changed = img_load(&img, filenames[fileidx]);
			}
			break;

		/* zooming */
		case XK_plus:
		case XK_equal:
			changed = img_zoom_in(&img);
			break;
		case XK_minus:
			changed = img_zoom_out(&img);
			break;

		/* panning */
		case XK_h:
		case XK_Left:
			changed = img_pan(&img, &win, PAN_LEFT);
			break;
		case XK_j:
		case XK_Down:
			changed = img_pan(&img, &win, PAN_DOWN);
			break;
		case XK_k:
		case XK_Up:
			changed = img_pan(&img, &win, PAN_UP);
			break;
		case XK_l:
		case XK_Right:
			changed = img_pan(&img, &win, PAN_RIGHT);
			break;

		/* rotation */
		case XK_less:
			changed = img_rotate_left(&img, &win);
			break;
		case XK_greater:
			changed = img_rotate_right(&img, &win);
			break;

		/* control window */
		case XK_f:
			win_toggle_fullscreen(&win);
			break;

		/* miscellaneous */
		case XK_a:
			changed = img_toggle_antialias(&img);
			break;
		case XK_r:
			changed = img_load(&img, filenames[fileidx]);
			break;
	}

	if (changed) {
		img_render(&img, &win);
		update_title();
		timeout = 0;
	}
}

void on_buttonpress(XEvent *ev) {
	int changed;
	unsigned int mask;

	if (!ev)
		return;

	mask = CLEANMASK(ev->xbutton.state);
	changed = 0;

	switch (ev->xbutton.button) {
		case Button1:
			if (fileidx + 1 < filecnt) {
				img_load(&img, filenames[++fileidx]);
				changed = 1;
			}
			break;
		case Button2:
			mox = ev->xbutton.x;
			moy = ev->xbutton.y;
			win_set_cursor(&win, CURSOR_HAND);
			break;
		case Button3:
			if (fileidx > 0) {
				img_load(&img, filenames[--fileidx]);
				changed = 1;
			}
			break;
		case Button4:
			if (mask == ControlMask)
				changed = img_zoom_in(&img);
			else if (mask == ShiftMask)
				changed = img_pan(&img, &win, PAN_LEFT);
			else
				changed = img_pan(&img, &win, PAN_UP);
			break;
		case Button5:
			if (mask == ControlMask)
				changed = img_zoom_out(&img);
			else if (mask == ShiftMask)
				changed = img_pan(&img, &win, PAN_RIGHT);
			else
				changed = img_pan(&img, &win, PAN_DOWN);
			break;
		case 6:
			changed = img_pan(&img, &win, PAN_LEFT);
			break;
		case 7:
			changed = img_pan(&img, &win, PAN_RIGHT);
			break;
	}

	if (changed) {
		img_render(&img, &win);
		update_title();
		timeout = 0;
	}
}

void on_buttonrelease(XEvent *ev) {
	if (!ev)
		return;

	if (ev->xbutton.button == Button2)
		win_set_cursor(&win, CURSOR_ARROW);
}

void on_motionnotify(XEvent *ev) {
	XMotionEvent *m;

	if (!ev)
		return;

	m = &ev->xmotion;
	
	if (m->x >= 0 && m->x <= win.w && m->y >= 0 && m->y <= win.h) {
		if (img_move(&img, &win, m->x - mox, m->y - moy))
			timeout = 1;

		mox = m->x;
		moy = m->y;
	}
}

void on_configurenotify(XEvent *ev) {
	if (!ev)
		return;

	if (win_configure(&win, &ev->xconfigure)) {
		img.checkpan = 1;
		timeout = 1;
	}
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

void check_append(const char *filename) {
	if (!filename)
		return;

	if (img_check(filename)) {
		if (fileidx == filecnt) {
			filecnt *= 2;
			filenames = (const char**) s_realloc(filenames,
																					 filecnt * sizeof(const char*));
		}
		filenames[fileidx++] = filename;
	}
}

void read_dir_rec(const char *dirname) {
	char *filename;
	const char **dirnames;
	int dircnt, diridx;
	unsigned char first;
	size_t len;
	DIR *dir;
	struct dirent *dentry;
	struct stat fstats;

	if (!dirname)
		return;

	dircnt = DNAME_CNT;
	diridx = first = 1;
	dirnames = (const char**) s_malloc(dircnt * sizeof(const char*));
	dirnames[0] = dirname;

	while (diridx > 0) {
		dirname = dirnames[--diridx];
		if (!(dir = opendir(dirname)))
			die("could not open directory: %s", dirname);
		while ((dentry = readdir(dir))) {
			if (!strcmp(dentry->d_name, ".") || !strcmp(dentry->d_name, ".."))
				continue;
			len = strlen(dirname) + strlen(dentry->d_name) + 2;
			filename = (char*) s_malloc(len * sizeof(char));
			snprintf(filename, len, "%s/%s", dirname, dentry->d_name);
			if (stat(filename, &fstats)) {
				warn("could not stat file: %s", filename);
				free(filename);
			} else if (S_ISDIR(fstats.st_mode)) {
				if (diridx == dircnt) {
					dircnt *= 2;
					dirnames = (const char**) s_realloc(dirnames,
					                                    dircnt * sizeof(const char*));
				}
				dirnames[diridx++] = filename;
			} else {
				check_append(filename);
			}
		}
		closedir(dir);
		if (!first)
			free((void*) dirname);
		else
			first = 0;
	}

	free(dirnames);
}
