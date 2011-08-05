/* sxiv: events.c
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

#define _GENERAL_CONFIG
#define _MAPPINGS_CONFIG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include "events.h"
#include "image.h"
#include "thumbs.h"
#include "types.h"
#include "util.h"
#include "window.h"
#include "config.h"

/* timeouts in milliseconds: */
enum {
	TO_WIN_RESIZE  = 75,
	TO_IMAGE_DRAG  = 1,
	TO_CURSOR_HIDE = 1500,
	TO_THUMBS_LOAD = 200
};

void cleanup();
void remove_file(int, unsigned char);
void load_image(int);
void update_title();

extern appmode_t mode;
extern img_t img;
extern tns_t tns;
extern win_t win;

extern char **filenames;
extern int filecnt, fileidx;

int timo_cursor;
int timo_redraw;
unsigned char dragging;
int mox, moy;

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
		fncnt++;

	if (!fncnt)
		return 0;

	ret = 0;
	fname = filenames[mode == MODE_NORMAL ? fileidx : tns.sel];
	fnlen = strlen(fname);
	cn = cmdline = (char*) s_malloc((strlen(cline) + fncnt * (fnlen + 2)) *
	                                sizeof(char));

	/* replace all '#' with filename */
	for (co = cline; *co; co++) {
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

void redraw() {
	if (mode == MODE_NORMAL) {
		img_render(&img, &win);
		if (timo_cursor)
			win_set_cursor(&win, CURSOR_ARROW);
		else if (!dragging)
			win_set_cursor(&win, CURSOR_NONE);
	} else {
		tns_render(&tns, &win);
	}
	update_title();
	timo_redraw = 0;
}

void on_keypress(XEvent *ev) {
	int i;
	XKeyEvent *kev;
	KeySym ksym;
	char key;

	if (!ev || ev->type != KeyPress)
		return;
	
	kev = &ev->xkey;
	XLookupString(kev, &key, 1, &ksym, NULL);

	if (EXT_COMMANDS && (CLEANMASK(kev->state) & ControlMask)) {
		for (i = 0; i < LEN(commands); i++) {
			if (commands[i].ksym == ksym) {
				win_set_cursor(&win, CURSOR_WATCH);
				if (run_command(commands[i].cmdline, commands[i].reload)) {
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

	for (i = 0; i < LEN(keys); i++) {
		if (ksym == keys[i].ksym && keys[i].handler) {
			if (keys[i].handler(ev, keys[i].arg))
				redraw();
			return;
		}
	}
}

void on_buttonpress(XEvent *ev) {
	int i, sel;
	XButtonEvent *bev;

	if (!ev || ev->type != ButtonPress)
		return;

	bev = &ev->xbutton;

	if (mode == MODE_NORMAL) {
		if (!dragging) {
			win_set_cursor(&win, CURSOR_ARROW);
			timo_cursor = TO_CURSOR_HIDE;
		}

		for (i = 0; i < LEN(buttons); i++) {
			if (CLEANMASK(bev->state) == CLEANMASK(buttons[i].mod) &&
			    bev->button == buttons[i].button && buttons[i].handler)
			{
				if (buttons[i].handler(ev, buttons[i].arg))
					redraw();
				return;
			}
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
					redraw();
					break;
				}
				break;
			case Button4:
			case Button5:
				if (tns_scroll(&tns, bev->button == Button4 ? DIR_UP : DIR_DOWN))
					redraw();
				break;
		}
	}
}

void on_motionnotify(XEvent *ev) {
	XMotionEvent *mev;

	if (!ev || ev->type != MotionNotify)
		return;

	mev = &ev->xmotion;

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

	dragging = 0;
	timo_cursor = mode == MODE_NORMAL ? TO_CURSOR_HIDE : 0;

	redraw();

	while (1) {
		if (mode == MODE_THUMBS && tns.cnt < filecnt) {
			/* load thumbnails */
			win_set_cursor(&win, CURSOR_WATCH);
			gettimeofday(&t0, 0);

			while (tns.cnt < filecnt && !XPending(win.env.dpy)) {
				if (tns_load(&tns, tns.cnt, filenames[tns.cnt], 0))
					tns.cnt++;
				else
					remove_file(tns.cnt, 0);
				gettimeofday(&t1, 0);
				if (TIMEDIFF(&t1, &t0) >= TO_THUMBS_LOAD)
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
			/* check active timeouts */
			gettimeofday(&t0, 0);
			timeout = MIN(timo_cursor + 1, timo_redraw + 1);
			MSEC_TO_TIMEVAL(timeout, &tt);
			xfd = ConnectionNumber(win.env.dpy);
			FD_ZERO(&fds);
			FD_SET(xfd, &fds);

			if (!XPending(win.env.dpy))
				select(xfd + 1, &fds, 0, 0, &tt);
			gettimeofday(&t1, 0);
			timeout = MIN(TIMEDIFF(&t1, &t0), timeout);

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
			if ((timo_cursor || timo_redraw) && !XPending(win.env.dpy))
				continue;
		}

		if (!XNextEvent(win.env.dpy, &ev)) {
			switch (ev.type) {
				case ButtonPress:
					on_buttonpress(&ev);
					break;
				case ButtonRelease:
					if (dragging) {
						dragging = 0;
						if (mode == MODE_NORMAL) {
							win_set_cursor(&win, CURSOR_ARROW);
							timo_cursor = TO_CURSOR_HIDE;
						}
					}
					break;
				case ClientMessage:
					if ((Atom) ev.xclient.data.l[0] == wm_delete_win)
						return;
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
				case KeyPress:
					on_keypress(&ev);
					break;
				case MotionNotify:
					if (dragging) {
						on_motionnotify(&ev);
					} else if (mode == MODE_NORMAL) {
						if (!timo_cursor)
							win_set_cursor(&win, CURSOR_ARROW);
						timo_cursor = TO_CURSOR_HIDE;
					}
					break;
			}
		}
	}
}


/* handler functions for key and button mappings: */

int quit(XEvent *e, arg_t a) {
	cleanup();
	exit(0);
}

int reload(XEvent *e, arg_t a) {
	if (mode == MODE_NORMAL) {
		load_image(fileidx);
		return 1;
	} else {
		return 0;
	}
}

int toggle_fullscreen(XEvent *e, arg_t a) {
	win_toggle_fullscreen(&win);
	if (mode == MODE_NORMAL)
		img.checkpan = 1;
	else
		tns.dirty = 1;
	timo_redraw = TO_WIN_RESIZE;
	return 0;
}

int toggle_antialias(XEvent *e, arg_t a) {
	if (mode == MODE_NORMAL) {
		img_toggle_antialias(&img);
		return 1;
	} else {
		return 0;
	}
}

int toggle_alpha(XEvent *e, arg_t a) {
	if (mode == MODE_NORMAL) {
		img.alpha ^= 1;
		return 1;
	} else {
		return 0;
	}
}

int switch_mode(XEvent *e, arg_t a) {
	if (mode == MODE_NORMAL) {
		if (!tns.thumbs)
			tns_init(&tns, filecnt);
		img_close(&img, 0);
		win_set_cursor(&win, CURSOR_ARROW);
		timo_cursor = 0;
		tns.sel = fileidx;
		tns.dirty = 1;
		mode = MODE_THUMBS;
	} else {
		timo_cursor = TO_CURSOR_HIDE;
		load_image(tns.sel);
		mode = MODE_NORMAL;
	}
	return 1;
}

int navigate(XEvent *e, arg_t n) {
	if (mode == MODE_NORMAL) {
		n += fileidx;
		if (n < 0)
			n = 0;
		if (n >= filecnt)
			n = filecnt - 1;

		if (n != fileidx) {
			load_image(n);
			return 1;
		}
	}
	return 0;
}

int first(XEvent *e, arg_t a) {
	if (mode == MODE_NORMAL && fileidx != 0) {
		load_image(0);
		return 1;
	} else if (mode == MODE_THUMBS && tns.sel != 0) {
		tns.sel = 0;
		tns.dirty = 1;
		return 1;
	} else {
		return 0;
	}
}

int last(XEvent *e, arg_t a) {
	if (mode == MODE_NORMAL && fileidx != filecnt - 1) {
		load_image(filecnt - 1);
		return 1;
	} else if (mode == MODE_THUMBS && tns.sel != tns.cnt - 1) {
		tns.sel = tns.cnt - 1;
		tns.dirty = 1;
		return 1;
	} else {
		return 0;
	}
}

int remove_image(XEvent *e, arg_t a) {
	if (mode == MODE_NORMAL) {
		remove_file(fileidx, 1);
		load_image(fileidx >= filecnt ? filecnt - 1 : fileidx);
		return 1;
	} else if (tns.sel < tns.cnt) {
		remove_file(tns.sel, 1);
		tns.dirty = 1;
		if (tns.sel >= tns.cnt)
			tns.sel = tns.cnt - 1;
		return 1;
	} else {
		return 0;
	}
}

int move(XEvent *e, arg_t dir) {
	if (mode == MODE_NORMAL)
		return img_pan(&img, &win, dir, 0);
	else
		return tns_move_selection(&tns, &win, dir);
}

int pan_screen(XEvent *e, arg_t dir) {
	if (mode == MODE_NORMAL)
		return img_pan(&img, &win, dir, 1);
	else
		return 0;
}

int pan_edge(XEvent *e, arg_t dir) {
	if (mode == MODE_NORMAL)
		return img_pan_edge(&img, &win, dir);
	else
		return 0;
}

int drag(XEvent *e, arg_t a) {
	if (mode == MODE_NORMAL) {
		mox = e->xbutton.x;
		moy = e->xbutton.y;
		win_set_cursor(&win, CURSOR_HAND);
		timo_cursor = 0;
		dragging = 1;
	}
	return 0;
}

int rotate(XEvent *e, arg_t dir) {
	if (mode == MODE_NORMAL) {
		if (dir == DIR_LEFT) {
			img_rotate_left(&img, &win);
			return 1;
		} else if (dir == DIR_RIGHT) {
			img_rotate_right(&img, &win);
			return 1;
		}
	}
	return 0;
}

int zoom(XEvent *e, arg_t scale) {
	if (mode != MODE_NORMAL)
		return 0;
	if (scale > 0)
		return img_zoom_in(&img, &win);
	else if (scale < 0)
		return img_zoom_out(&img, &win);
	else
		return img_zoom(&img, &win, 1.0);
}

int fit_to_win(XEvent *e, arg_t ret) {
	if (mode == MODE_NORMAL) {
		if ((ret = img_fit_win(&img, &win)))
			img_center(&img, &win);
		return ret;
	} else {
		return 0;
	}
}

int fit_to_img(XEvent *e, arg_t ret) {
	int x, y;
	unsigned int w, h;

	if (mode == MODE_NORMAL) {
		x = MAX(0, win.x + img.x);
		y = MAX(0, win.y + img.y);
		w = img.w * img.zoom;
		h = img.h * img.zoom;
		if ((ret = win_moveresize(&win, x, y, w, h))) {
			img.x = x - win.x;
			img.y = y - win.y;
		}
		return ret;
	} else {
		return 0;
	}
}
