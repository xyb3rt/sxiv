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
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "image.h"
#include "options.h"
#include "thumbs.h"
#include "util.h"
#include "window.h"

typedef enum {
	MODE_NORMAL = 0,
	MODE_THUMBS
} appmode_t;

void update_title();
int check_append(const char*);
void read_dir_rec(const char*);
void run();

appmode_t mode;
img_t img;
tns_t tns;
win_t win;

#define DNAME_CNT 512
#define FNAME_CNT 1024
const char **filenames;
int filecnt, fileidx;
size_t filesize;

#define TITLE_LEN 256
char win_title[TITLE_LEN];

void cleanup() {
	static int in = 0;

	if (!in++) {
		img_close(&img);
		img_free(&img);
		tns_free(&tns, &win);
		win_close(&win);
	}
}

int load_image(int new) {
	struct stat fstats;

	if (new >= 0 && new < filecnt) {
		img_close(&img);
		fileidx = new;
		if (!stat(filenames[fileidx], &fstats))
			filesize = fstats.st_size;
		else
			filesize = 0;
		return img_load(&img, filenames[fileidx]);
	} else {
		return 0;
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

	if (options->recursive || options->from_stdin)
		filecnt = FNAME_CNT;
	else
		filecnt = options->filecnt;

	filenames = (const char**) s_malloc(filecnt * sizeof(const char*));
	fileidx = 0;

	if (options->from_stdin) {
		while ((filename = readline(stdin))) {
			if (!*filename || !check_append(filename))
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

	if (options->thumbnails) {
		mode = MODE_THUMBS;
		tns_init(&tns, filecnt);
		win_clear(&win);
		win_draw(&win);
	} else {
		mode = MODE_NORMAL;
		tns.thumbs = NULL;
		load_image(fileidx);
		img_render(&img, &win);
	}

	update_title();
	run();
	cleanup();

	return 0;
}

void update_title() {
	int n;
	float size;
	const char *unit;

	if (mode == MODE_THUMBS) {
		n = snprintf(win_title, TITLE_LEN, "sxiv: [%d/%d] %s",
		             tns.cnt ? tns.sel + 1 : 0, tns.cnt,
		             tns.cnt ? filenames[tns.sel] : "");
	} else {
		if (img.im) {
			size = filesize;
			size_readable(&size, &unit);
			n = snprintf(win_title, TITLE_LEN, "sxiv: [%d/%d] <%d%%> (%.2f%s) %s",
			             fileidx + 1, filecnt, (int) (img.zoom * 100.0), size, unit,
			             filenames[fileidx]);
		} else {
			n = snprintf(win_title, TITLE_LEN, "sxiv: [%d/%d] broken: %s",
			             fileidx + 1, filecnt, filenames[fileidx]);
		}
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

int fncmp(const void *a, const void *b) {
	return strcoll(*((char* const*) a), *((char* const*) b));
}

void read_dir_rec(const char *dirname) {
	char *filename;
	const char **dirnames;
	int dircnt, diridx;
	int fcnt, fstart;
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

	fcnt = 0;
	fstart = fileidx;

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
					if (check_append(filename))
						++fcnt;
					else
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

	if (fcnt > 1)
		qsort(filenames + fstart, fcnt, sizeof(char*), fncmp);

	free(dirnames);
}


/* event handling */

#define TO_WIN_RESIZE  75000;
#define TO_IMAGE_DRAG  1000;
#define TO_CURSOR_HIDE 1500000;
#define TO_THUMBS_LOAD 75000;
int timo_cursor;
int timo_redraw;

unsigned char drag;
int mox, moy;

void redraw() {
	if (mode == MODE_NORMAL)
		img_render(&img, &win);
	else
		tns_render(&tns, &win);
	update_title();
	timo_redraw = 0;
}

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

	if (mode == MODE_NORMAL) {
		switch (ksym) {
			/* navigate image list */
			case XK_n:
			case XK_space:
				if (fileidx + 1 < filecnt)
					changed = load_image(fileidx + 1);
				break;
			case XK_p:
			case XK_BackSpace:
				if (fileidx > 0)
					changed = load_image(fileidx - 1);
				break;
			case XK_bracketleft:
				if (fileidx != 0)
					changed = load_image(MAX(0, fileidx - 10));
				break;
			case XK_bracketright:
				if (fileidx != filecnt - 1)
					changed = load_image(MIN(fileidx + 10, filecnt - 1));
				break;
			case XK_g:
				if (fileidx != 0)
					changed = load_image(0);
				break;
			case XK_G:
				if (fileidx != filecnt - 1)
					changed = load_image(filecnt - 1);
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
			case XK_W:
				x = MAX(0, win.x + img.x);
				y = MAX(0, win.y + img.y);
				w = img.w * img.zoom;
				h = img.h * img.zoom;
				if ((changed = win_moveresize(&win, x, y, w, h))) {
					img.x = x - win.x;
					img.y = y - win.y;
				}
				break;

			/* switch to thumbnail mode */
			case XK_Return:
				if (!tns.thumbs)
					tns_init(&tns, filecnt);
				img_close(&img);
				mode = MODE_THUMBS;
				win_set_cursor(&win, CURSOR_ARROW);
				timo_cursor = 0;
				tns.sel = fileidx;
				changed = tns.dirty = 1;
				break;

			/* miscellaneous */
			case XK_a:
				img_toggle_antialias(&img);
				changed = 1;
				break;
			case XK_r:
				changed = load_image(fileidx);
				break;
		}
	} else {
		/* thumbnail mode */
		switch (ksym) {
			/* open selected image */
			case XK_Return:
				load_image(tns.sel);
				mode = MODE_NORMAL;
				win_set_cursor(&win, CURSOR_NONE);
				changed = 1;
				break;

			/* move selection */
			case XK_h:
			case XK_Left:
				changed = tns_move_selection(&tns, &win, TNS_LEFT);
				break;
			case XK_j:
			case XK_Down:
				changed = tns_move_selection(&tns, &win, TNS_DOWN);
				break;
			case XK_k:
			case XK_Up:
				changed = tns_move_selection(&tns, &win, TNS_UP);
				break;
			case XK_l:
			case XK_Right:
				changed = tns_move_selection(&tns, &win, TNS_RIGHT);
				break;
			case XK_g:
				if (tns.sel != 0) {
					tns.sel = 0;
					changed = tns.dirty = 1;
				}
				break;
			case XK_G:
				if (tns.sel != tns.cnt - 1) {
					tns.sel = tns.cnt - 1;
					changed = tns.dirty = 1;
				}
		}
	}

	/* common key mappings */
	switch (ksym) {
		case XK_Escape:
			cleanup();
			exit(2);
		case XK_q:
			cleanup();
			exit(0);

		case XK_f:
			win_toggle_fullscreen(&win);
			/* render on next configurenotify */
			break;
	}

	if (changed)
		redraw();
}

void on_buttonpress(XButtonEvent *bev) {
	int changed, sel;
	unsigned int mask;

	if (!bev)
		return;

	mask = CLEANMASK(bev->state);
	changed = 0;

	if (mode == MODE_NORMAL) {
		switch (bev->button) {
			case Button1:
				if (fileidx + 1 < filecnt)
					changed = load_image(fileidx + 1);
				break;
			case Button2:
				mox = bev->x;
				moy = bev->y;
				win_set_cursor(&win, CURSOR_HAND);
				timo_cursor = 0;
				drag = 1;
				break;
			case Button3:
				if (fileidx > 0)
					changed = load_image(fileidx - 1);
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
	} else {
		/* thumbnail mode */
		switch (bev->button) {
			case Button1:
				if ((sel = tns_translate(&tns, bev->x, bev->y)) >= 0) {
					if (sel == tns.sel) {
						load_image(tns.sel);
						mode = MODE_NORMAL;
						timo_cursor = TO_CURSOR_HIDE;
					} else {
						tns_highlight(&tns, &win, tns.sel, False);
						tns_highlight(&tns, &win, sel, True);
						tns.sel = sel;
					}
					changed = 1;
					break;
				}
				break;
			case Button4:
				changed = tns_scroll(&tns, TNS_UP);
				break;
			case Button5:
				changed = tns_scroll(&tns, TNS_DOWN);
				break;
		}
	}

	if (changed)
		redraw();
}

void on_motionnotify(XMotionEvent *mev) {
	if (!mev)
		return;

	if (mev->x >= 0 && mev->x <= win.w && mev->y >= 0 && mev->y <= win.h) {
		if (img_move(&img, &win, mev->x - mox, mev->y - moy))
			timo_redraw = TO_IMAGE_DRAG;

		mox = mev->x;
		moy = mev->y;
	}
}

void run() {
	int xfd, timeout;
	fd_set fds;
	struct timeval tt, t0, t1;
	XEvent ev;

	timo_cursor = timo_redraw = 0;
	drag = 0;

	if (mode == MODE_NORMAL)
		timo_cursor = TO_CURSOR_HIDE;

	while (1) {
		if (mode == MODE_THUMBS && tns.cnt < filecnt) {
			win_set_cursor(&win, CURSOR_WATCH);
			gettimeofday(&t0, 0);

			while (!XPending(win.env.dpy) && tns.cnt < filecnt) {
				/* tns.cnt is increased inside tns_load */
				tns_load(&tns, &win, tns.cnt, filenames[tns.cnt]);
				gettimeofday(&t1, 0);
				if (TV_TO_DOUBLE(t1) - TV_TO_DOUBLE(t0) >= 0.25)
					break;
			}
			if (tns.cnt == filecnt)
				win_set_cursor(&win, CURSOR_ARROW);
			if (!XPending(win.env.dpy)) {
				redraw();
				continue;
			} else {
				timo_redraw = TO_THUMBS_LOAD;
			}
		} else if (timo_cursor || timo_redraw) {
			gettimeofday(&t0, 0);
			if (timo_cursor && timo_redraw)
				timeout = MIN(timo_cursor, timo_redraw);
			else if (timo_cursor)
				timeout = timo_cursor;
			else
				timeout = timo_redraw;
			tt.tv_sec = timeout / 1000000;
			tt.tv_usec = timeout % 1000000;
			xfd = ConnectionNumber(win.env.dpy);
			FD_ZERO(&fds);
			FD_SET(xfd, &fds);

			if (!XPending(win.env.dpy))
				select(xfd + 1, &fds, 0, 0, &tt);
			gettimeofday(&t1, 0);
			timeout = MIN((TV_TO_DOUBLE(t1) - TV_TO_DOUBLE(t0)) * 1000000, timeout);

			/* timeouts fired? */
			if (timo_cursor) {
				timo_cursor = MAX(0, timo_cursor - timeout);
				if (!timo_cursor)
					win_set_cursor(&win, CURSOR_NONE);
			}
			if (timo_redraw) {
				timo_redraw = MAX(0, timo_redraw - timeout);
				if (!timo_redraw)
					redraw();
			}
			if (!XPending(win.env.dpy) && (timo_cursor || timo_redraw))
				continue;
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
					if (ev.xbutton.button == Button2) {
						drag = 0;
						if (mode == MODE_NORMAL) {
							win_set_cursor(&win, CURSOR_ARROW);
							timo_cursor = TO_CURSOR_HIDE;
						}
					}
					break;
				case MotionNotify:
					if (drag) {
						on_motionnotify(&ev.xmotion);
					} else if (mode == MODE_NORMAL) {
						if (!timo_cursor)
							win_set_cursor(&win, CURSOR_ARROW);
						timo_cursor = TO_CURSOR_HIDE;
					}
					break;
				case ConfigureNotify:
					if (win_configure(&win, &ev.xconfigure)) {
						timo_redraw = TO_WIN_RESIZE;
						if (mode == MODE_NORMAL)
							img.checkpan = 1;
						else
							tns.dirty = 1;
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
