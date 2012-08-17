/* sxiv: main.c
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
#define _MAPPINGS_CONFIG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "commands.h"
#include "image.h"
#include "options.h"
#include "thumbs.h"
#include "types.h"
#include "util.h"
#include "window.h"
#include "config.h"

enum {
	INFO_STR_LEN = 256,
	FILENAME_CNT = 1024
};

typedef struct {
	struct timeval when;
	bool active;
	timeout_f handler;
} timeout_t;

/* timeout handler functions: */
void redraw(void);
void reset_cursor(void);
void animate(void);
void clear_resize(void);

appmode_t mode;
img_t img;
tns_t tns;
win_t win;

fileinfo_t *files;
int filecnt, fileidx;
int alternate;
size_t filesize;

int prefix;

bool resized = false;

char win_bar_l[INFO_STR_LEN];
char win_bar_r[INFO_STR_LEN];
char win_title[INFO_STR_LEN];

timeout_t timeouts[] = {
	{ { 0, 0 }, false, redraw },
	{ { 0, 0 }, false, reset_cursor },
	{ { 0, 0 }, false, animate },
	{ { 0, 0 }, false, clear_resize },
};

void cleanup(void) {
	static bool in = false;

	if (!in) {
		in = true;
		img_close(&img, false);
		tns_free(&tns);
		win_close(&win);
	}
}

void check_add_file(char *filename) {
	const char *bn;

	if (filename == NULL || *filename == '\0')
		return;

	if (access(filename, R_OK) < 0) {
		warn("could not open file: %s", filename);
		return;
	}

	if (fileidx == filecnt) {
		filecnt *= 2;
		files = (fileinfo_t*) s_realloc(files, filecnt * sizeof(fileinfo_t));
	}
	if (*filename != '/') {
		files[fileidx].path = absolute_path(filename);
		if (files[fileidx].path == NULL) {
			warn("could not get absolute path of file: %s\n", filename);
			return;
		}
	}
	files[fileidx].loaded = false;
	files[fileidx].name = s_strdup(filename);
	if (*filename == '/')
		files[fileidx].path = files[fileidx].name;
	if ((bn = strrchr(files[fileidx].name , '/')) != NULL && bn[1] != '\0')
		files[fileidx].base = ++bn;
	else
		files[fileidx].base = files[fileidx].name;
	fileidx++;
}

void remove_file(int n, bool manual) {
	if (n < 0 || n >= filecnt)
		return;

	if (filecnt == 1) {
		if (!manual)
			fprintf(stderr, "sxiv: no more files to display, aborting\n");
		cleanup();
		exit(manual ? EXIT_SUCCESS : EXIT_FAILURE);
	}

	if (files[n].path != files[n].name)
		free((void*) files[n].path);
	free((void*) files[n].name);

	if (n + 1 < filecnt)
		memmove(files + n, files + n + 1, (filecnt - n - 1) * sizeof(fileinfo_t));
	if (n + 1 < tns.cnt) {
		memmove(tns.thumbs + n, tns.thumbs + n + 1, (tns.cnt - n - 1) *
		        sizeof(thumb_t));
		memset(tns.thumbs + tns.cnt - 1, 0, sizeof(thumb_t));
	}

	filecnt--;
	if (n < tns.cnt)
		tns.cnt--;
}

void set_timeout(timeout_f handler, int time, bool overwrite) {
	int i;

	for (i = 0; i < ARRLEN(timeouts); i++) {
		if (timeouts[i].handler == handler) {
			if (!timeouts[i].active || overwrite) {
				gettimeofday(&timeouts[i].when, 0);
				TV_ADD_MSEC(&timeouts[i].when, time);
				timeouts[i].active = true;
			}
			return;
		}
	}
}

void reset_timeout(timeout_f handler) {
	int i;

	for (i = 0; i < ARRLEN(timeouts); i++) {
		if (timeouts[i].handler == handler) {
			timeouts[i].active = false;
			return;
		}
	}
}

bool check_timeouts(struct timeval *t) {
	int i = 0, tdiff, tmin = -1;
	struct timeval now;

	gettimeofday(&now, 0);
	while (i < ARRLEN(timeouts)) {
		if (timeouts[i].active) {
			tdiff = TV_DIFF(&timeouts[i].when, &now);
			if (tdiff <= 0) {
				timeouts[i].active = false;
				if (timeouts[i].handler != NULL)
					timeouts[i].handler();
				i = tmin = -1;
			} else if (tmin < 0 || tdiff < tmin) {
				tmin = tdiff;
			}
		}
		i++;
	}
	if (tmin > 0 && t != NULL)
		TV_SET_MSEC(t, tmin);
	return tmin > 0;
}

void load_image(int new) {
	struct stat fstats;

	if (new < 0 || new >= filecnt)
		return;

	win_set_cursor(&win, CURSOR_WATCH);

	img_close(&img, false);
	while (!img_load(&img, &files[new])) {
		remove_file(new, false);
		if (new >= filecnt)
			new = filecnt - 1;
	}

	files[new].loaded = true;
	alternate = fileidx;
	fileidx = new;
	if (stat(files[new].path, &fstats) == 0)
		filesize = fstats.st_size;
	else
		filesize = 0;

	if (img.multi.cnt > 0 && img.multi.animate)
		set_timeout(animate, img.multi.frames[img.multi.sel].delay, true);
	else
		reset_timeout(animate);
}

void update_info(void) {
	int i, fw, pw, fi, ln, rn;
	char frame_info[16];
	const char *size_unit;
	float size = filesize;

	pw = 0;
	for (i = filecnt; i > 0; i /= 10)
		pw++;

	if (mode == MODE_THUMB) {
		if (tns.cnt != filecnt) {
			snprintf(win_bar_l, sizeof win_bar_l, "Loading... %0*d/%d",
			         pw, tns.cnt, filecnt);
		} else {
			fi = snprintf(win_bar_l, sizeof win_bar_l, "%0*d/%d%s",
			              pw, tns.sel + 1, filecnt, BAR_SEPARATOR);
			ln = snprintf(win_bar_l + fi, sizeof win_bar_l - fi, "%s",
			              files[tns.sel].name) + fi;
			if (win_textwidth(win_bar_l, ln, true) > win.w)
				snprintf(win_bar_l + fi, sizeof win_bar_l - fi, "%s",
				         files[tns.sel].base);
		}
		win_set_title(&win, "sxiv");
		win_set_bar_info(&win, win_bar_l, NULL);
	} else {
		size_readable(&size, &size_unit);
		if (img.multi.cnt > 0) {
			fw = 0;
			for (i = img.multi.cnt; i > 0; i /= 10)
				fw++;
			snprintf(frame_info, sizeof frame_info, "%s%0*d/%d",
			         BAR_SEPARATOR, fw, img.multi.sel+1, img.multi.cnt);
		} else {
			frame_info[0] = '\0';
		}
		fi = snprintf(win_bar_l, sizeof win_bar_l, "%0*d/%d%s",
		              pw, fileidx + 1, filecnt, BAR_SEPARATOR);
		ln = snprintf(win_bar_l + fi, sizeof win_bar_l - fi, "%s",
		              files[fileidx].name) + fi;
		rn = snprintf(win_bar_r, sizeof win_bar_r, "%.2f%s%s%dx%d%s%3d%%%s",
		              size, size_unit, BAR_SEPARATOR, img.w, img.h, BAR_SEPARATOR,
		              (int) (img.zoom * 100.0), frame_info);

		if (win_textwidth(win_bar_l, ln, true) +
		    win_textwidth(win_bar_r, rn, true) > win.w)
		{
			snprintf(win_bar_l + fi, sizeof win_bar_l - fi, "%s",
			         files[fileidx].base);
		}
		win_set_bar_info(&win, win_bar_l, win_bar_r);

		snprintf(win_title, sizeof win_title, "sxiv - %s", files[fileidx].name);
		win_set_title(&win, win_title);
	}
}

void redraw(void) {
	if (mode == MODE_IMAGE)
		img_render(&img);
	else
		tns_render(&tns);
	update_info();
	win_draw(&win);
	reset_timeout(redraw);
	reset_cursor();
}

void reset_cursor(void) {
	int i;
	cursor_t cursor = CURSOR_NONE;

	if (mode == MODE_IMAGE) {
		for (i = 0; i < ARRLEN(timeouts); i++) {
			if (timeouts[i].handler == reset_cursor) {
				if (timeouts[i].active)
					cursor = CURSOR_ARROW;
				break;
			}
		}
	} else {
		if (tns.cnt != filecnt)
			cursor = CURSOR_WATCH;
		else
			cursor = CURSOR_ARROW;
	}
	win_set_cursor(&win, cursor);
}

void animate(void) {
	if (img_frame_animate(&img, false)) {
		redraw();
		set_timeout(animate, img.multi.frames[img.multi.sel].delay, true);
	}
}

void clear_resize(void) {
	resized = false;
}

bool keymask(const keymap_t *k, unsigned int state) {
	return (k->ctrl ? ControlMask : 0) == (state & ControlMask);
}

bool buttonmask(const button_t *b, unsigned int state) {
	return ((b->ctrl ? ControlMask : 0) | (b->shift ? ShiftMask : 0)) ==
	       (state & (ControlMask | ShiftMask));
}

void on_keypress(XKeyEvent *kev) {
	int i;
	KeySym ksym;
	char key;

	if (kev == NULL)
		return;

	XLookupString(kev, &key, 1, &ksym, NULL);

	if ((ksym == XK_Escape || (key >= '0' && key <= '9')) &&
	    (kev->state & ControlMask) == 0)
	{
		/* number prefix for commands */
		prefix = ksym == XK_Escape ? 0 : prefix * 10 + (int) (key - '0');
		return;
	}

	for (i = 0; i < ARRLEN(keys); i++) {
		if (keys[i].ksym == ksym && keymask(&keys[i], kev->state)) {
			if (keys[i].cmd != NULL && keys[i].cmd(keys[i].arg))
				redraw();
			prefix = 0;
			return;
		}
	}
}

void on_buttonpress(XButtonEvent *bev) {
	int i, sel;

	if (bev == NULL)
		return;

	if (mode == MODE_IMAGE) {
		win_set_cursor(&win, CURSOR_ARROW);
		set_timeout(reset_cursor, TO_CURSOR_HIDE, true);

		for (i = 0; i < ARRLEN(buttons); i++) {
			if (buttons[i].button == bev->button &&
			    buttonmask(&buttons[i], bev->state))
			{
				if (buttons[i].cmd != NULL && buttons[i].cmd(buttons[i].arg))
					redraw();
				return;
			}
		}
	} else {
		/* thumbnail mode (hard-coded) */
		switch (bev->button) {
			case Button1:
				if ((sel = tns_translate(&tns, bev->x, bev->y)) >= 0) {
					if (sel == tns.sel) {
						mode = MODE_IMAGE;
						set_timeout(reset_cursor, TO_CURSOR_HIDE, true);
						load_image(tns.sel);
					} else {
						tns_highlight(&tns, tns.sel, false);
						tns_highlight(&tns, sel, true);
						tns.sel = sel;
					}
					redraw();
					break;
				}
				break;
			case Button4:
			case Button5:
				if (tns_scroll(&tns, bev->button == Button4 ? DIR_UP : DIR_DOWN,
				               (bev->state & ControlMask) != 0))
					redraw();
				break;
		}
	}
}

void run(void) {
	int xfd;
	fd_set fds;
	struct timeval timeout;
	XEvent ev, nextev;
	unsigned int qlen;

	redraw();

	while (true) {
		while (mode == MODE_THUMB && tns.cnt < filecnt &&
		       XPending(win.env.dpy) == 0)
		{
			/* load thumbnails */
			set_timeout(redraw, TO_REDRAW_THUMBS, false);
			if (tns_load(&tns, tns.cnt, &files[tns.cnt], false, false)) {
				tns.cnt++;
			} else {
				remove_file(tns.cnt, false);
				if (tns.sel >= tns.cnt)
					tns.sel--;
			}
			if (tns.cnt == filecnt)
				redraw();
			else
				check_timeouts(NULL);
		}

		while (XPending(win.env.dpy) == 0 && check_timeouts(&timeout)) {
			/* wait for timeouts */
			xfd = ConnectionNumber(win.env.dpy);
			FD_ZERO(&fds);
			FD_SET(xfd, &fds);
			select(xfd + 1, &fds, 0, 0, &timeout);
		}

		do {
			XNextEvent(win.env.dpy, &ev);
			qlen = XEventsQueued(win.env.dpy, QueuedAlready);
			if (qlen > 0)
				XPeekEvent(win.env.dpy, &nextev);
		} while (qlen > 0 && ev.type == nextev.type);

		switch (ev.type) {
			/* handle events */
			case ButtonPress:
				on_buttonpress(&ev.xbutton);
				break;
			case ClientMessage:
				if ((Atom) ev.xclient.data.l[0] == wm_delete_win)
					return;
				break;
			case ConfigureNotify:
				if (win_configure(&win, &ev.xconfigure)) {
					if (mode == MODE_IMAGE)
						img.checkpan = true;
					else
						tns.dirty = true;
					if (!resized || win.fullscreen) {
						redraw();
						set_timeout(clear_resize, TO_REDRAW_RESIZE, false);
						resized = true;
					} else {
						set_timeout(redraw, TO_REDRAW_RESIZE, false);
					}
				}
				break;
			case KeyPress:
				if (qlen == 0 || ev.xkey.keycode != nextev.xkey.keycode)
					on_keypress(&ev.xkey);
				break;
			case MotionNotify:
				if (mode == MODE_IMAGE) {
					win_set_cursor(&win, CURSOR_ARROW);
					set_timeout(reset_cursor, TO_CURSOR_HIDE, true);
				}
				break;
		}
	}
}

int fncmp(const void *a, const void *b) {
	return strcoll(((fileinfo_t*) a)->name, ((fileinfo_t*) b)->name);
}

int main(int argc, char **argv) {
	int i, start;
	size_t n;
	ssize_t len;
	char *filename;
	struct stat fstats;
	r_dir_t dir;

	parse_options(argc, argv);

	if (options->clean_cache) {
		tns_init(&tns, 0, NULL);
		tns_clean_cache(&tns);
		exit(EXIT_SUCCESS);
	}

	if (options->filecnt == 0) {
		print_usage();
		exit(EXIT_FAILURE);
	}

	if (options->recursive || options->from_stdin)
		filecnt = FILENAME_CNT;
	else
		filecnt = options->filecnt;

	files = (fileinfo_t*) s_malloc(filecnt * sizeof(fileinfo_t));
	fileidx = 0;

	/* build file list: */
	if (options->from_stdin) {
		filename = NULL;
		while ((len = get_line(&filename, &n, stdin)) > 0) {
			if (filename[len-1] == '\n')
				filename[len-1] = '\0';
			check_add_file(filename);
		}
		if (filename != NULL)
			free(filename);
	} else {
		for (i = 0; i < options->filecnt; i++) {
			filename = options->filenames[i];

			if (stat(filename, &fstats) < 0) {
				warn("could not stat file: %s", filename);
				continue;
			}
			if (!S_ISDIR(fstats.st_mode)) {
				check_add_file(filename);
			} else {
				if (!options->recursive) {
					warn("ignoring directory: %s", filename);
					continue;
				}
				if (r_opendir(&dir, filename) < 0) {
					warn("could not open directory: %s", filename);
					continue;
				}
				start = fileidx;
				while ((filename = r_readdir(&dir)) != NULL) {
					check_add_file(filename);
					free((void*) filename);
				}
				r_closedir(&dir);
				if (fileidx - start > 1)
					qsort(files + start, fileidx - start, sizeof(fileinfo_t), fncmp);
			}
		}
	}

	if (fileidx == 0) {
		fprintf(stderr, "sxiv: no valid image file given, aborting\n");
		exit(EXIT_FAILURE);
	}

	filecnt = fileidx;
	fileidx = options->startnum < filecnt ? options->startnum : 0;

	win_init(&win);
	img_init(&img, &win);

	if (options->thumb_mode) {
		mode = MODE_THUMB;
		tns_init(&tns, filecnt, &win);
		while (!tns_load(&tns, 0, &files[0], false, false))
			remove_file(0, false);
		tns.cnt = 1;
	} else {
		mode = MODE_IMAGE;
		tns.thumbs = NULL;
		load_image(fileidx);
	}

	win_open(&win);

	run();
	cleanup();

	return 0;
}
