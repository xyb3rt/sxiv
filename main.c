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

void update_title();
int check_append(const char*);
void read_dir_rec(const char*);
void run();

img_t img;
win_t win;

#define DNAME_CNT 512
#define FNAME_CNT 4096
const char **filenames;
int filecnt, fileidx;
size_t filesize;

#define TITLE_LEN 256
char win_title[TITLE_LEN];

void cleanup() {
	static int in = 0;

	if (!in++) {
		img_free(&img);
		win_close(&win);
	}
}

int load_image() {
	struct stat fstats;

	if (!stat(filenames[fileidx], &fstats))
		filesize = fstats.st_size;
	else
		filesize = 0;

	return img_load(&img, filenames[fileidx]);
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

	if (options->recursive || options->from_stdin)
		filecnt = FNAME_CNT;
	else
		filecnt = options->filecnt;

	filenames = (const char**) s_malloc(filecnt * sizeof(const char*));
	fileidx = 0;

	if (options->from_stdin) {
		while ((filename = readline(stdin))) {
			if (!check_append(filename))
				free((void*) filename);
		}
	} else {
		for (i = 0; i < options->filecnt; ++i) {
			filename = options->filenames[i];
			if (!stat(filename, &fstats) && S_ISDIR(fstats.st_mode)) {
				if (options->recursive)
					read_dir_rec(filename);
				else
					warn("ignoring directory: %s", filename);
			} else {
				check_append(filename);
			}
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

	load_image();
	img_render(&img, &win);
	update_title();

	run();

	cleanup();

	return 0;
}

void update_title() {
	int n;
	float size;
	const char *unit;

	if (img.valid) {
		size = filesize;
		size_readable(&size, &unit);
		n = snprintf(win_title, TITLE_LEN, "sxiv: [%d/%d] <%d%%> (%.2f%s) %s",
								 fileidx + 1, filecnt, (int) (img.zoom * 100.0), size, unit,
								 filenames[fileidx]);
	} else {
		n = snprintf(win_title, TITLE_LEN, "sxiv: [%d/%d] broken: %s",
		             fileidx + 1, filecnt, filenames[fileidx]);
	}

	if (n >= TITLE_LEN) {
		win_title[TITLE_LEN - 2] = '.';
		win_title[TITLE_LEN - 3] = '.';
		win_title[TITLE_LEN - 4] = '.';
	}

	win_set_title(&win, win_title);
}

int check_append(const char *filename) {
	if (!filename)
		return 0;

	if (img_check(filename)) {
		if (fileidx == filecnt) {
			filecnt *= 2;
			filenames = (const char**) s_realloc(filenames,
																					 filecnt * sizeof(const char*));
		}
		filenames[fileidx++] = filename;
		return 1;
	} else {
		return 0;
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
		if (!(dir = opendir(dirname))) {
			warn("could not open directory: %s", dirname);
		} else {
			while ((dentry = readdir(dir))) {
				if (!strcmp(dentry->d_name, ".") || !strcmp(dentry->d_name, ".."))
					continue;

				len = strlen(dirname) + strlen(dentry->d_name) + 2;
				filename = (char*) s_malloc(len * sizeof(char));
				snprintf(filename, len, "%s/%s", dirname, dentry->d_name);

				if (!stat(filename, &fstats) && S_ISDIR(fstats.st_mode)) {
					if (diridx == dircnt) {
						dircnt *= 2;
						dirnames = (const char**) s_realloc(dirnames,
																								dircnt * sizeof(const char*));
					}
					dirnames[diridx++] = filename;
				} else {
					if (!check_append(filename))
						free(filename);
				}
			}
			closedir(dir);
		}

		if (!first)
			free((void*) dirname);
		else
			first = 0;
	}

	free(dirnames);
}


/* event handling */

unsigned char timeout;
int mox, moy;

void on_keypress(XKeyEvent *kev) {
	int x, y;
	unsigned int w, h;
	char key;
	KeySym ksym;
	int changed;

	if (!kev)
		return;
	
	XLookupString(kev, &key, 1, &ksym, NULL);
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
				++fileidx;
				changed = load_image();
			}
			break;
		case XK_p:
		case XK_BackSpace:
			if (fileidx > 0) {
				--fileidx;
				changed = load_image();
			}
			break;
		case XK_bracketleft:
			if (fileidx != 0) {
				fileidx = MAX(0, fileidx - 10);
				changed = load_image();
			}
			break;
		case XK_bracketright:
			if (fileidx != filecnt - 1) {
				fileidx = MIN(fileidx + 10, filecnt - 1);
				changed = load_image();
			}
			break;
		case XK_g:
			if (fileidx != 0) {
				fileidx = 0;
				changed = load_image();
			}
			break;
		case XK_G:
			if (fileidx != filecnt - 1) {
				fileidx = filecnt - 1;
				changed = load_image();
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
		case XK_0:
			changed = img_zoom(&img, 1.0);
			break;
		case XK_w:
			if ((changed = img_fit_win(&img, &win)))
				img_center(&img, &win);
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
			img_rotate_left(&img, &win);
			changed = 1;
			break;
		case XK_greater:
			img_rotate_right(&img, &win);
			changed = 1;
			break;

		/* control window */
		case XK_f:
			win_toggle_fullscreen(&win);
			/* render on next configurenotify */
			break;
		case XK_W:
			x = win.x + img.x;
			y = win.y + img.y;
			w = img.w * img.zoom;
			h = img.h * img.zoom;
			if ((changed = win_moveresize(&win, x, y, w, h))) {
				img.x = x - win.x;
				img.y = y - win.y;
			}
			break;

		/* miscellaneous */
		case XK_a:
			img_toggle_antialias(&img);
			changed = 1;
			break;
		case XK_r:
			changed = load_image();
			break;
	}

	if (changed) {
		img_render(&img, &win);
		update_title();
		timeout = 0;
	}
}

void on_buttonpress(XButtonEvent *bev) {
	int changed;
	unsigned int mask;

	if (!bev)
		return;

	mask = CLEANMASK(bev->state);
	changed = 0;

	switch (bev->button) {
		case Button1:
			if (fileidx + 1 < filecnt) {
				++fileidx;
				changed = load_image();
			}
			break;
		case Button2:
			mox = bev->x;
			moy = bev->y;
			win_set_cursor(&win, CURSOR_HAND);
			break;
		case Button3:
			if (fileidx > 0) {
				--fileidx;
				changed = load_image();
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

void on_motionnotify(XMotionEvent *mev) {
	if (!mev)
		return;

	if (mev->x >= 0 && mev->x <= win.w && mev->y >= 0 && mev->y <= win.h) {
		if (img_move(&img, &win, mev->x - mox, mev->y - moy))
			timeout = 1;

		mox = mev->x;
		moy = mev->y;
	}
}

void run() {
	int xfd;
	fd_set fds;
	struct timeval t;
	XEvent ev;

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

		if (!XNextEvent(win.env.dpy, &ev)) {
			switch (ev.type) {
				case KeyPress:
					on_keypress(&ev.xkey);
					break;
				case ButtonPress:
					on_buttonpress(&ev.xbutton);
					break;
				case ButtonRelease:
					if (ev.xbutton.button == Button2)
						win_set_cursor(&win, CURSOR_ARROW);
					break;
				case MotionNotify:
					on_motionnotify(&ev.xmotion);
					break;
				case ConfigureNotify:
					if (win_configure(&win, &ev.xconfigure)) {
						img.checkpan = 1;
						timeout = 1;
					}
					break;
				case ClientMessage:
					if ((Atom) ev.xclient.data.l[0] == wm_delete_win)
						return;
					break;
			}
		}
	}
}
