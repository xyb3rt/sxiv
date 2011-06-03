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

#define _XOPEN_SOURCE 700

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "image.h"
#include "options.h"
#include "thumbs.h"
#include "util.h"
#include "window.h"

#define FNAME_CNT 1024
#define TITLE_LEN 256

typedef enum {
	MODE_NORMAL = 0,
	MODE_THUMBS
} appmode_t;

typedef struct {
	KeySym ksym;
	Bool reload;
	const char *cmdline;
} command_t;

#define MAIN_C
#include "config.h"

void run();

appmode_t mode;
img_t img;
tns_t tns;
win_t win;

char **filenames;
int filecnt, fileidx;
size_t filesize;

char win_title[TITLE_LEN];

void cleanup() {
	static int in = 0;

	if (!in++) {
		img_close(&img, 0);
		tns_free(&tns);
		win_close(&win);
	}
}

void remove_file(int n, unsigned char silent) {
	if (n < 0 || n >= filecnt)
		return;

	if (filecnt == 1) {
		if (!silent)
			fprintf(stderr, "sxiv: no more files to display, aborting\n");
		cleanup();
		exit(!silent);
	}

	if (n + 1 < filecnt)
		memmove(filenames + n, filenames + n + 1, (filecnt - n - 1) *
		        sizeof(char*));
	if (n + 1 < tns.cnt) {
		memmove(tns.thumbs + n, tns.thumbs + n + 1, (tns.cnt - n - 1) *
		        sizeof(thumb_t));
		memset(tns.thumbs + tns.cnt - 1, 0, sizeof(thumb_t));
	}

	--filecnt;
	if (n < tns.cnt)
		--tns.cnt;
}

int load_image(int new) {
	struct stat fstats;

	if (new >= 0 && new < filecnt) {
		win_set_cursor(&win, CURSOR_WATCH);
		img_close(&img, 0);
		
		while (!img_load(&img, filenames[new])) {
			remove_file(new, 0);
			if (new >= filecnt)
				new = filecnt - 1;
		}
		fileidx = new;
		if (!stat(filenames[new], &fstats))
			filesize = fstats.st_size;
		else
			filesize = 0;

		/* cursor is reset in redraw() */
	}
	return 1;
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
		size = filesize;
		size_readable(&size, &unit);
		n = snprintf(win_title, TITLE_LEN,
		             "sxiv: [%d/%d] <%d%%> <%dx%d> (%.2f%s) %s",
		             fileidx + 1, filecnt, (int) (img.zoom * 100.0), img.w, img.h,
		             size, unit, filenames[fileidx]);
	}

	if (n >= TITLE_LEN) {
		win_title[TITLE_LEN - 2] = '.';
		win_title[TITLE_LEN - 3] = '.';
		win_title[TITLE_LEN - 4] = '.';
	}

	win_set_title(&win, win_title);
}

int check_append(char *filename) {
	if (!filename)
		return 0;

	if (access(filename, R_OK)) {
		warn("could not open file: %s", filename);
		return 0;
	} else {
		if (fileidx == filecnt) {
			filecnt *= 2;
			filenames = (char**) s_realloc(filenames, filecnt * sizeof(char*));
		}
		filenames[fileidx++] = filename;
		return 1;
	}
}

int fncmp(const void *a, const void *b) {
	return strcoll(*((char* const*) a), *((char* const*) b));
}

int main(int argc, char **argv) {
	int i, len, start;
	size_t n;
	char *filename = NULL;
	struct stat fstats;
	r_dir_t dir;

	parse_options(argc, argv);

	if (options->clean_cache) {
		tns_init(&tns, 0);
		tns_clean_cache(&tns);
		exit(0);
	}

	if (!options->filecnt) {
		print_usage();
		exit(1);
	}

	if (options->recursive || options->from_stdin)
		filecnt = FNAME_CNT;
	else
		filecnt = options->filecnt;

	filenames = (char**) s_malloc(filecnt * sizeof(char*));
	fileidx = 0;

	if (options->from_stdin) {
		while ((len = getline(&filename, &n, stdin)) > 0) {
			if (filename[len-1] == '\n')
				filename[len-1] = '\0';
			if (!*filename || !check_append(filename))
				free(filename);
			filename = NULL;
		}
	} else {
		for (i = 0; i < options->filecnt; ++i) {
			filename = options->filenames[i];

			if (stat(filename, &fstats) || !S_ISDIR(fstats.st_mode)) {
				check_append(filename);
			} else {
				if (!options->recursive) {
					warn("ignoring directory: %s", filename);
					continue;
				}
				if (r_opendir(&dir, filename)) {
					warn("could not open directory: %s", filename);
					continue;
				}
				start = fileidx;
				while ((filename = r_readdir(&dir))) {
					if (!check_append(filename))
						free((void*) filename);
				}
				r_closedir(&dir);
				if (fileidx - start > 1)
					qsort(filenames + start, fileidx - start, sizeof(char*), fncmp);
			}
		}
	}

	if (!fileidx) {
		fprintf(stderr, "sxiv: no valid image file given, aborting\n");
		exit(1);
	}

	filecnt = fileidx;
	fileidx = options->startnum < filecnt ? options->startnum : filecnt - 1;

	win_init(&win);
	img_init(&img, &win);

	if (options->thumbnails) {
		mode = MODE_THUMBS;
		tns_init(&tns, filecnt);
		while (!tns_load(&tns, 0, filenames[0], 0))
			remove_file(0, 0);
		tns.cnt = 1;
	} else {
		mode = MODE_NORMAL;
		tns.thumbs = NULL;
		load_image(fileidx);
	}

	win_open(&win);
	
	run();
	cleanup();

	return 0;
}

#if EXT_COMMANDS
int run_command(const char *cline, Bool reload) {
	int fncnt, fnlen;
	char *cn, *cmdline;
	const char *co, *fname;
	pid_t pid;
	int ret, status;

	if (!cline || !*cline)
		return 0;

	fncnt = 0;
	co = cline - 1;
	while ((co = strchr(co + 1, '#')))
		++fncnt;

	if (!fncnt)
		return 0;

	ret = 0;
	fname = filenames[mode == MODE_NORMAL ? fileidx : tns.sel];
	fnlen = strlen(fname);
	cn = cmdline = (char*) s_malloc((strlen(cline) + fncnt * (fnlen + 2)) *
	                                sizeof(char));

	/* replace all '#' with filename */
	for (co = cline; *co; ++co) {
		if (*co == '#') {
			*cn++ = '"';
			strcpy(cn, fname);
			cn += fnlen;
			*cn++ = '"';
		} else {
			*cn++ = *co;
		}
	}
	*cn = '\0';

	if ((pid = fork()) == 0) {
		execlp("/bin/sh", "/bin/sh", "-c", cmdline, NULL);
		warn("could not exec: /bin/sh");
		exit(1);
	} else if (pid < 0) {
		warn("could not fork. command line was: %s", cmdline);
	} else if (reload) {
		waitpid(pid, &status, 0);
		if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
			ret = 1;
		else
			warn("child exited with non-zero return value: %d. command line was: %s",
			     WEXITSTATUS(status), cmdline);
	}
	
	free(cmdline);
	return ret;
}
#endif /* EXT_COMMANDS */


/* event handling */

#define TO_WIN_RESIZE  75000
#define TO_IMAGE_DRAG  1000
#define TO_CURSOR_HIDE 1500000
#define TO_THUMBS_LOAD 75000

int timo_cursor;
int timo_redraw;
unsigned char drag;
int mox, moy;

void redraw() {
	if (mode == MODE_NORMAL) {
		img_render(&img, &win);
		if (timo_cursor)
			win_set_cursor(&win, CURSOR_ARROW);
		else if (!drag)
			win_set_cursor(&win, CURSOR_NONE);
	} else {
		tns_render(&tns, &win);
	}
	update_title();
	timo_redraw = 0;
}

void on_keypress(XKeyEvent *kev) {
	int x, y;
	unsigned int w, h;
	char key;
	KeySym ksym;
	int changed, ctrl;

	if (!kev)
		return;
	
	XLookupString(kev, &key, 1, &ksym, NULL);
	changed = 0;
	ctrl = CLEANMASK(kev->state) & ControlMask;

#if EXT_COMMANDS
	/* external commands from commands.h */
	if (ctrl) {
		for (x = 0; x < LEN(commands); ++x) {
			if (commands[x].ksym == ksym) {
				win_set_cursor(&win, CURSOR_WATCH);
				if (run_command(commands[x].cmdline, commands[x].reload)) {
					if (mode == MODE_NORMAL) {
						if (fileidx < tns.cnt)
							tns_load(&tns, fileidx, filenames[fileidx], 1);
						img_close(&img, 1);
						load_image(fileidx);
					} else {
						if (!tns_load(&tns, tns.sel, filenames[tns.sel], 0)) {
							remove_file(tns.sel, 0);
							tns.dirty = 1;
							if (tns.sel >= tns.cnt)
								tns.sel = tns.cnt - 1;
						}
					}
					redraw();
				}
				if (mode == MODE_THUMBS)
					win_set_cursor(&win, CURSOR_ARROW);
				else if (!timo_cursor)
					win_set_cursor(&win, CURSOR_NONE);
				return;
			}
		}
	}
#endif

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
				changed = img_zoom_in(&img, &win);
				break;
			case XK_minus:
				changed = img_zoom_out(&img, &win);
				break;
			case XK_0:
				changed = img_zoom(&img, &win, 1.0);
				break;
			case XK_w:
				if ((changed = img_fit_win(&img, &win)))
					img_center(&img, &win);
				break;

			/* panning */
			case XK_h:
			case XK_Left:
				changed = img_pan(&img, &win, PAN_LEFT, ctrl);
				break;
			case XK_j:
			case XK_Down:
				changed = img_pan(&img, &win, PAN_DOWN, ctrl);
				break;
			case XK_k:
			case XK_Up:
				changed = img_pan(&img, &win, PAN_UP, ctrl);
				break;
			case XK_l:
			case XK_Right:
				changed = img_pan(&img, &win, PAN_RIGHT, ctrl);
				break;
			case XK_Prior:
				changed = img_pan(&img, &win, PAN_UP, 1);
				break;
			case XK_Next:
				changed = img_pan(&img, &win, PAN_DOWN, 1);
				break;

			case XK_H:
				changed = img_pan_edge(&img, &win, PAN_LEFT);
				break;
			case XK_J:
				changed = img_pan_edge(&img, &win, PAN_DOWN);
				break;
			case XK_K:
				changed = img_pan_edge(&img, &win, PAN_UP);
				break;
			case XK_L:
				changed = img_pan_edge(&img, &win, PAN_RIGHT);
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
				img_close(&img, 0);
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
			case XK_A:
				img.alpha ^= 1;
				changed = 1;
				break;
			case XK_D:
				remove_file(fileidx, 1);
				changed = load_image(fileidx >= filecnt ? filecnt - 1 : fileidx);
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
				break;

			/* miscellaneous */
			case XK_D:
				if (tns.sel < tns.cnt) {
					remove_file(tns.sel, 1);
					changed = tns.dirty = 1;
					if (tns.sel >= tns.cnt)
						tns.sel = tns.cnt - 1;
				}
				break;
		}
	}

	/* common key mappings */
	switch (ksym) {
		case XK_q:
			cleanup();
			exit(0);
		case XK_f:
			win_toggle_fullscreen(&win);
			if (mode == MODE_NORMAL)
				img.checkpan = 1;
			else
				tns.dirty = 1;
			timo_redraw = TO_WIN_RESIZE;
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
		if (!drag) {
			win_set_cursor(&win, CURSOR_ARROW);
			timo_cursor = TO_CURSOR_HIDE;
		}

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
					changed = img_zoom_in(&img, &win);
				else if (mask == ShiftMask)
					changed = img_pan(&img, &win, PAN_LEFT, 0);
				else
					changed = img_pan(&img, &win, PAN_UP, 0);
				break;
			case Button5:
				if (mask == ControlMask)
					changed = img_zoom_out(&img, &win);
				else if (mask == ShiftMask)
					changed = img_pan(&img, &win, PAN_RIGHT, 0);
				else
					changed = img_pan(&img, &win, PAN_DOWN, 0);
				break;
			case 6:
				changed = img_pan(&img, &win, PAN_LEFT, 0);
				break;
			case 7:
				changed = img_pan(&img, &win, PAN_RIGHT, 0);
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

	drag = 0;
	timo_cursor = mode == MODE_NORMAL ? TO_CURSOR_HIDE : 0;

	redraw();

	while (1) {
		if (mode == MODE_THUMBS && tns.cnt < filecnt) {
			win_set_cursor(&win, CURSOR_WATCH);
			gettimeofday(&t0, 0);

			while (tns.cnt < filecnt && !XPending(win.env.dpy)) {
				if (tns_load(&tns, tns.cnt, filenames[tns.cnt], 0))
					++tns.cnt;
				else
					remove_file(tns.cnt, 0);
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
