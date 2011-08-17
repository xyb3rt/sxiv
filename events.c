/* sxiv: events.c
 * Copyright (c) 2011 Bert Muennich <muennich at informatik.hu-berlin.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, 5th Floor, Boston, MA 02110-1301, USA.
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

extern fileinfo_t *files;
extern int filecnt, fileidx;

int timo_cursor;
int timo_redraw;

void redraw() {
	if (mode == MODE_NORMAL) {
		img_render(&img, &win);
		if (timo_cursor)
			win_set_cursor(&win, CURSOR_ARROW);
		else
			win_set_cursor(&win, CURSOR_NONE);
	} else {
		tns_render(&tns, &win);
	}
	update_title();
	timo_redraw = 0;
}

Bool keymask(const keymap_t *k, unsigned int state) {
	return (k->ctrl ? ControlMask : 0) == (state & ControlMask);
}

Bool buttonmask(const button_t *b, unsigned int state) {
	return ((b->ctrl ? ControlMask : 0) | (b->shift ? ShiftMask : 0)) ==
	       (state & (ControlMask | ShiftMask));
}

void on_keypress(XKeyEvent *kev) {
	int i;
	KeySym ksym;
	char key;

	if (!kev)
		return;

	XLookupString(kev, &key, 1, &ksym, NULL);

	for (i = 0; i < LEN(keys); i++) {
		if (keymask(&keys[i], kev->state) && ksym == keys[i].ksym) {
			if (keys[i].handler && keys[i].handler(keys[i].arg))
				redraw();
			return;
		}
	}
}

void on_buttonpress(XButtonEvent *bev) {
	int i, sel;

	if (!bev)
		return;

	if (mode == MODE_NORMAL) {
		win_set_cursor(&win, CURSOR_ARROW);
		timo_cursor = TO_CURSOR_HIDE;

		for (i = 0; i < LEN(buttons); i++) {
			if (buttonmask(&buttons[i], bev->state) &&
			    bev->button == buttons[i].button)
			{
				if (buttons[i].handler && buttons[i].handler(buttons[i].arg))
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

void run() {
	int xfd, timeout;
	fd_set fds;
	struct timeval tt, t0, t1;
	XEvent ev;

	timo_cursor = mode == MODE_NORMAL ? TO_CURSOR_HIDE : 0;

	redraw();

	while (1) {
		if (mode == MODE_THUMBS && tns.cnt < filecnt) {
			/* load thumbnails */
			win_set_cursor(&win, CURSOR_WATCH);
			gettimeofday(&t0, 0);

			while (tns.cnt < filecnt && !XPending(win.env.dpy)) {
				if (tns_load(&tns, tns.cnt, &files[tns.cnt], 0))
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
			/* handle events */
			switch (ev.type) {
				case ButtonPress:
					on_buttonpress(&ev.xbutton);
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
					on_keypress(&ev.xkey);
					break;
				case MotionNotify:
					if (!timo_cursor)
						win_set_cursor(&win, CURSOR_ARROW);
					timo_cursor = TO_CURSOR_HIDE;
					break;
			}
		}
	}
}


/* handler functions for key and button mappings: */

int quit(arg_t a) {
	cleanup();
	exit(0);
}

int reload(arg_t a) {
	if (mode == MODE_NORMAL) {
		load_image(fileidx);
		return 1;
	} else {
		return 0;
	}
}

int toggle_fullscreen(arg_t a) {
	win_toggle_fullscreen(&win);
	if (mode == MODE_NORMAL)
		img.checkpan = 1;
	else
		tns.dirty = 1;
	timo_redraw = TO_WIN_RESIZE;
	return 0;
}

int toggle_antialias(arg_t a) {
	if (mode == MODE_NORMAL) {
		img_toggle_antialias(&img);
		return 1;
	} else {
		return 0;
	}
}

int toggle_alpha(arg_t a) {
	if (mode == MODE_NORMAL) {
		img.alpha ^= 1;
		return 1;
	} else {
		return 0;
	}
}

int switch_mode(arg_t a) {
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

int navigate(arg_t a) {
	int n = (int) a;

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

int first(arg_t a) {
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

int last(arg_t a) {
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

int remove_image(arg_t a) {
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

int move(arg_t a) {
	direction_t dir = (direction_t) a;

	if (mode == MODE_NORMAL)
		return img_pan(&img, &win, dir, 0);
	else
		return tns_move_selection(&tns, &win, dir);
}

int pan_screen(arg_t a) {
	direction_t dir = (direction_t) a;

	if (mode == MODE_NORMAL)
		return img_pan(&img, &win, dir, 1);
	else
		return 0;
}

int pan_edge(arg_t a) {
	direction_t dir = (direction_t) a;

	if (mode == MODE_NORMAL)
		return img_pan_edge(&img, &win, dir);
	else
		return 0;
}

/* Xlib helper function for drag() */
Bool is_motionnotify(Display *d, XEvent *e, XPointer a) {
	return e != NULL && e->type == MotionNotify;
}

int drag(arg_t a) {
	int dx = 0, dy = 0, i, ox, oy, x, y;
	unsigned int ui;
	Bool dragging = True, next = False;
	XEvent e;
	Window w;

	if (mode != MODE_NORMAL)
		return 0;
	if (!XQueryPointer(win.env.dpy, win.xwin, &w, &w, &i, &i, &ox, &oy, &ui))
		return 0;
	
	win_set_cursor(&win, CURSOR_HAND);

	while (dragging) {
		if (!next)
			XMaskEvent(win.env.dpy,
			           ButtonPressMask | ButtonReleaseMask | PointerMotionMask, &e);
		switch (e.type) {
			case ButtonPress:
			case ButtonRelease:
				dragging = False;
				break;
			case MotionNotify:
				x = e.xmotion.x;
				y = e.xmotion.y;
				if (x >= 0 && x <= win.w && y >= 0 && y <= win.h) {
					dx += x - ox;
					dy += y - oy;
				}
				ox = x;
				oy = y;
				break;
		}
		if (dragging)
			next = XCheckIfEvent(win.env.dpy, &e, is_motionnotify, None);
		if ((!dragging || !next) && (dx != 0 || dy != 0)) {
			if (img_move(&img, &win, dx, dy))
				img_render(&img, &win);
			dx = dy = 0;
		}
	}
	
	win_set_cursor(&win, CURSOR_ARROW);
	timo_cursor = TO_CURSOR_HIDE;
	timo_redraw = 0;

	return 0;
}

int rotate(arg_t a) {
	direction_t dir = (direction_t) a;

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

int zoom(arg_t a) {
	int scale = (int) a;

	if (mode != MODE_NORMAL)
		return 0;
	if (scale > 0)
		return img_zoom_in(&img, &win);
	else if (scale < 0)
		return img_zoom_out(&img, &win);
	else
		return img_zoom(&img, &win, 1.0);
}

int fit_to_win(arg_t a) {
	int ret;

	if (mode == MODE_NORMAL) {
		if ((ret = img_fit_win(&img, &win)))
			img_center(&img, &win);
		return ret;
	} else {
		return 0;
	}
}

int fit_to_img(arg_t a) {
	int ret, x, y;
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

int open_with(arg_t a) {
	const char *prog = (const char*) a;
	pid_t pid;

	if (!prog || !*prog)
		return 0;

	if((pid = fork()) == 0) {
		execlp(prog, prog,
		       files[mode == MODE_NORMAL ? fileidx : tns.sel].path, NULL);
		warn("could not exec: %s", prog);
		exit(1);
	} else if (pid < 0) {
		warn("could not for. program was: %s", prog);
	}
	
	return 0;
}

int run_command(arg_t a) {
	const char *cline = (const char*) a;
	char *cn, *cmdline;
	const char *co, *fpath;
	int fpcnt, fplen, status;
	pid_t pid;

	if (!cline || !*cline)
		return 0;

	/* build command line: */
	fpcnt = 0;
	co = cline - 1;
	while ((co = strchr(co + 1, '#')))
		fpcnt++;
	if (!fpcnt)
		return 0;
	fpath = files[mode == MODE_NORMAL ? fileidx : tns.sel].path;
	fplen = strlen(fpath);
	cn = cmdline = (char*) s_malloc((strlen(cline) + fpcnt * (fplen + 2)) *
	                                sizeof(char));
	/* replace all '#' with filename: */
	for (co = cline; *co; co++) {
		if (*co == '#') {
			*cn++ = '"';
			strcpy(cn, fpath);
			cn += fplen;
			*cn++ = '"';
		} else {
			*cn++ = *co;
		}
	}
	*cn = '\0';

	win_set_cursor(&win, CURSOR_WATCH);

	if ((pid = fork()) == 0) {
		execl("/bin/sh", "/bin/sh", "-c", cmdline, NULL);
		warn("could not exec: /bin/sh");
		exit(1);
	} else if (pid < 0) {
		warn("could not fork. command line was: %s", cmdline);
		goto end;
	}

	waitpid(pid, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		warn("child exited with non-zero return value: %d. command line was: %s",
		     WEXITSTATUS(status), cmdline);
	
	if (mode == MODE_NORMAL) {
		if (fileidx < tns.cnt)
			tns_load(&tns, fileidx, &files[fileidx], 1);
		img_close(&img, 1);
		load_image(fileidx);
	} else {
		if (!tns_load(&tns, tns.sel, &files[tns.sel], 0)) {
			remove_file(tns.sel, 0);
			tns.dirty = 1;
			if (tns.sel >= tns.cnt)
				tns.sel = tns.cnt - 1;
		}
	}

end:
	if (mode == MODE_THUMBS)
		win_set_cursor(&win, CURSOR_ARROW);
	/* else: cursor is reset in redraw() */

	free(cmdline);

	return 1;
}
