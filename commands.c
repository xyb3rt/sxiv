/* sxiv: commands.c
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

#define _POSIX_C_SOURCE 200112L /* for setenv(3) */
#include <stdlib.h>

#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "commands.h"
#include "image.h"
#include "thumbs.h"
#include "types.h"
#include "util.h"

void cleanup();
void remove_file(int, unsigned char);
void load_image(int);
void redraw();
void hide_cursor();
void animate();
void set_timeout(timeout_f, int, int);
void reset_timeout(timeout_f);

extern appmode_t mode;
extern img_t img;
extern tns_t tns;
extern win_t win;

extern fileinfo_t *files;
extern int filecnt, fileidx;

int it_quit(arg_t a) {
	cleanup();
	exit(0);
}

int it_switch_mode(arg_t a) {
	if (mode == MODE_IMAGE) {
		if (!tns.thumbs)
			tns_init(&tns, filecnt);
		img_close(&img, 0);
		win_set_cursor(&win, CURSOR_ARROW);
		reset_timeout(hide_cursor);
		tns.sel = fileidx;
		tns.dirty = 1;
		mode = MODE_THUMB;
	} else {
		load_image(tns.sel);
		mode = MODE_IMAGE;
	}
	return 1;
}

int it_toggle_fullscreen(arg_t a) {
	win_toggle_fullscreen(&win);
	set_timeout(redraw, TO_REDRAW_RESIZE, 0);
	if (mode == MODE_IMAGE)
		img.checkpan = 1;
	else
		tns.dirty = 1;
	return 0;
}

int it_reload_image(arg_t a) {
	if (mode == MODE_IMAGE) {
		load_image(fileidx);
	} else if (!tns_load(&tns, tns.sel, &files[tns.sel], True, False)) {
		remove_file(tns.sel, 0);
		tns.dirty = 1;
		if (tns.sel >= tns.cnt)
			tns.sel = tns.cnt - 1;
	}
	return 1;
}

int it_remove_image(arg_t a) {
	if (mode == MODE_IMAGE) {
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

int i_navigate(arg_t a) {
	int n = (int) a;

	if (mode == MODE_IMAGE) {
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

int it_first(arg_t a) {
	if (mode == MODE_IMAGE && fileidx != 0) {
		load_image(0);
		return 1;
	} else if (mode == MODE_THUMB && tns.sel != 0) {
		tns.sel = 0;
		tns.dirty = 1;
		return 1;
	} else {
		return 0;
	}
}

int it_last(arg_t a) {
	if (mode == MODE_IMAGE && fileidx != filecnt - 1) {
		load_image(filecnt - 1);
		return 1;
	} else if (mode == MODE_THUMB && tns.sel != tns.cnt - 1) {
		tns.sel = tns.cnt - 1;
		tns.dirty = 1;
		return 1;
	} else {
		return 0;
	}
}

int i_navigate_frame(arg_t a) {
	if (mode == MODE_IMAGE && !img.multi.animate)
		return img_frame_navigate(&img, (int) a);
	else
		return 0;
}

int i_toggle_animation(arg_t a) {
	int delay;

	if (mode != MODE_IMAGE)
		return 0;

	if (img.multi.animate) {
		reset_timeout(animate);
		img.multi.animate = 0;
		return 0;
	} else {
		delay = img_frame_animate(&img, 1);
		set_timeout(animate, delay, 1);
		return 1;
	}
}

int it_move(arg_t a) {
	direction_t dir = (direction_t) a;

	if (mode == MODE_IMAGE)
		return img_pan(&img, &win, dir, 0);
	else
		return tns_move_selection(&tns, &win, dir);
}

int i_pan_screen(arg_t a) {
	direction_t dir = (direction_t) a;

	if (mode == MODE_IMAGE)
		return img_pan(&img, &win, dir, 1);
	else
		return 0;
}

int i_pan_edge(arg_t a) {
	direction_t dir = (direction_t) a;

	if (mode == MODE_IMAGE)
		return img_pan_edge(&img, &win, dir);
	else
		return 0;
}

/* Xlib helper function for i_drag() */
Bool is_motionnotify(Display *d, XEvent *e, XPointer a) {
	return e != NULL && e->type == MotionNotify;
}

int i_drag(arg_t a) {
	int dx = 0, dy = 0, i, ox, oy, x, y;
	unsigned int ui;
	Bool dragging = True, next = False;
	XEvent e;
	Window w;

	if (mode != MODE_IMAGE)
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
	set_timeout(hide_cursor, TO_CURSOR_HIDE, 1);
	reset_timeout(redraw);

	return 0;
}

int i_zoom(arg_t a) {
	int scale = (int) a;

	if (mode != MODE_IMAGE)
		return 0;

	if (scale > 0)
		return img_zoom_in(&img, &win);
	else if (scale < 0)
		return img_zoom_out(&img, &win);
	else
		return img_zoom(&img, &win, 1.0);
}

int i_fit_to_win(arg_t a) {
	int ret;

	if (mode == MODE_IMAGE) {
		if ((ret = img_fit_win(&img, &win)))
			img_center(&img, &win);
		return ret;
	} else {
		return 0;
	}
}

int i_fit_to_img(arg_t a) {
	int ret, x, y;
	unsigned int w, h;

	if (mode == MODE_IMAGE) {
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

int i_rotate(arg_t a) {
	direction_t dir = (direction_t) a;

	if (mode == MODE_IMAGE) {
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

int i_toggle_antialias(arg_t a) {
	if (mode == MODE_IMAGE) {
		img_toggle_antialias(&img);
		return 1;
	} else {
		return 0;
	}
}

int i_toggle_alpha(arg_t a) {
	if (mode == MODE_IMAGE) {
		img.alpha ^= 1;
		return 1;
	} else {
		return 0;
	}
}

int it_open_with(arg_t a) {
	const char *prog = (const char*) a;
	pid_t pid;

	if (!prog || !*prog)
		return 0;

	if((pid = fork()) == 0) {
		execlp(prog, prog,
		       files[mode == MODE_IMAGE ? fileidx : tns.sel].path, NULL);
		warn("could not exec: %s", prog);
		exit(1);
	} else if (pid < 0) {
		warn("could not fork. program was: %s", prog);
	}
	
	return 0;
}

int it_shell_cmd(arg_t a) {
	int n, status;
	const char *cmdline = (const char*) a;
	pid_t pid;

	if (!cmdline || !*cmdline)
		return 0;

	n = mode == MODE_IMAGE ? fileidx : tns.sel;

	if (setenv("SXIV_IMG", files[n].path, 1) < 0) {
		warn("could not change env.-variable: SXIV_IMG. command line was: %s",
		     cmdline);
		return 0;
	}

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
	
	if (mode == MODE_IMAGE) {
		img_close(&img, 1);
		load_image(fileidx);
	}
	if (!tns_load(&tns, n, &files[n], True, mode == MODE_IMAGE) &&
	    mode == MODE_THUMB)
	{
		remove_file(tns.sel, 0);
		tns.dirty = 1;
		if (tns.sel >= tns.cnt)
			tns.sel = tns.cnt - 1;
	}

end:
	if (mode == MODE_THUMB)
		win_set_cursor(&win, CURSOR_ARROW);
	/* else: cursor gets reset in redraw() */

	return 1;
}
