/* Copyright 2011, 2012, 2014 Bert Muennich
 *
 * This file is part of sxiv.
 *
 * sxiv is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * sxiv is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with sxiv.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _POSIX_C_SOURCE 200112L
#define _IMAGE_CONFIG

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "commands.h"
#include "image.h"
#include "options.h"
#include "thumbs.h"
#include "util.h"
#include "config.h"

void cleanup(void);
void remove_file(int, bool);
void load_image(int);
void open_info(void);
void redraw(void);
void reset_cursor(void);
void animate(void);
void slideshow(void);
void set_timeout(timeout_f, int, bool);
void reset_timeout(timeout_f);

extern appmode_t mode;
extern img_t img;
extern tns_t tns;
extern win_t win;

extern fileinfo_t *files;
extern int filecnt, fileidx;
extern int alternate;
extern int markcnt;

extern int prefix;
extern bool extprefix;

const int ss_delays[] = {
	1, 2, 3, 5, 10, 15, 20, 30, 60, 120, 180, 300, 600
};

bool cg_quit(arg_t a)
{
	unsigned int i;

	if (options->to_stdout && markcnt > 0) {
		for (i = 0; i < filecnt; i++) {
			if (files[i].marked)
				printf("%s\n", files[i].name);
		}
	}
	cleanup();
	exit(EXIT_SUCCESS);
}

bool cg_switch_mode(arg_t a)
{
	if (mode == MODE_IMAGE) {
		if (tns.thumbs == NULL)
			tns_init(&tns, files, filecnt, &fileidx, &win);
		img_close(&img, false);
		reset_timeout(reset_cursor);
		if (img.ss.on) {
			img.ss.on = false;
			reset_timeout(slideshow);
		}
		tns.dirty = true;
		mode = MODE_THUMB;
	} else {
		load_image(fileidx);
		mode = MODE_IMAGE;
	}
	return true;
}

bool cg_toggle_fullscreen(arg_t a)
{
	win_toggle_fullscreen(&win);
	/* redraw after next ConfigureNotify event */
	set_timeout(redraw, TO_REDRAW_RESIZE, false);
	if (mode == MODE_IMAGE)
		img.checkpan = img.dirty = true;
	else
		tns.dirty = true;
	return false;
}

bool cg_toggle_bar(arg_t a)
{
	win_toggle_bar(&win);
	if (mode == MODE_IMAGE) {
		img.checkpan = img.dirty = true;
		if (win.bar.h > 0)
			open_info();
	} else {
		tns.dirty = true;
	}
	return true;
}

bool cg_prefix_external(arg_t a)
{
	extprefix = true;
	return false;
}

bool cg_reload_image(arg_t a)
{
	if (mode == MODE_IMAGE) {
		load_image(fileidx);
	} else {
		win_set_cursor(&win, CURSOR_WATCH);
		if (!tns_load(&tns, fileidx, true)) {
			remove_file(fileidx, false);
			tns.dirty = true;
		}
	}
	return true;
}

bool cg_remove_image(arg_t a)
{
	if (mode == MODE_IMAGE) {
		remove_file(fileidx, true);
		load_image(fileidx >= filecnt ? filecnt - 1 : fileidx);
		return true;
	} else if (fileidx < tns.cnt) {
		remove_file(fileidx, true);
		tns.dirty = true;
		return true;
	} else {
		return false;
	}
}

bool cg_first(arg_t a)
{
	if (mode == MODE_IMAGE && fileidx != 0) {
		load_image(0);
		return true;
	} else if (mode == MODE_THUMB && fileidx != 0) {
		fileidx = 0;
		tns.dirty = true;
		return true;
	} else {
		return false;
	}
}

bool cg_n_or_last(arg_t a)
{
	int n = prefix != 0 && prefix - 1 < filecnt ? prefix - 1 : filecnt - 1;

	if (mode == MODE_IMAGE && fileidx != n) {
		load_image(n);
		return true;
	} else if (mode == MODE_THUMB && fileidx != n) {
		fileidx = n;
		tns.dirty = true;
		return true;
	} else {
		return false;
	}
}

bool cg_scroll_screen(arg_t a)
{
	direction_t dir = (direction_t) a;

	if (mode == MODE_IMAGE)
		return img_pan(&img, dir, -1);
	else
		return tns_scroll(&tns, dir, true);
}

bool cg_toggle_image_mark(arg_t a)
{
	files[fileidx].marked = !files[fileidx].marked;
	markcnt += files[fileidx].marked ? 1 : -1;
	if (mode == MODE_THUMB)
		tns_mark(&tns, fileidx, files[fileidx].marked);
	return true;
}

bool cg_reverse_marks(arg_t a)
{
	int i;

	for (i = 0; i < filecnt; i++) {
		files[i].marked = !files[i].marked;
		markcnt += files[i].marked ? 1 : -1;
	}
	if (mode == MODE_THUMB)
		tns.dirty = true;
	return true;
}

bool cg_navigate_marked(arg_t a)
{
	long n = (long) a;
	int d, i;
	int cnt = mode == MODE_IMAGE ? filecnt : tns.cnt, new = fileidx;
	
	if (prefix > 0)
		n *= prefix;
	d = n > 0 ? 1 : -1;
	for (i = fileidx + d; n != 0 && i >= 0 && i < cnt; i += d) {
		if (files[i].marked) {
			n -= d;
			new = i;
		}
	}
	if (new != fileidx) {
		if (mode == MODE_IMAGE) {
			load_image(new);
		} else {
			fileidx = new;
			tns.dirty = true;
		}
		return true;
	} else {
		return false;
	}
}

bool ci_navigate(arg_t a)
{
	long n = (long) a;

	if (prefix > 0)
		n *= prefix;
	n += fileidx;
	if (n < 0)
		n = 0;
	if (n >= filecnt)
		n = filecnt - 1;

	if (n != fileidx) {
		load_image(n);
		return true;
	} else {
		return false;
	}
}

bool ci_alternate(arg_t a)
{
	load_image(alternate);
	return true;
}

bool ci_navigate_frame(arg_t a)
{
	return !img.multi.animate && img_frame_navigate(&img, (long) a);
}

bool ci_toggle_animation(arg_t a)
{
	bool dirty = false;

	img.multi.animate = !img.multi.animate;

	if (img.multi.animate) {
		dirty = img_frame_animate(&img, true);
		set_timeout(animate, img.multi.frames[img.multi.sel].delay, true);
	} else {
		reset_timeout(animate);
	}
	return dirty;
}

bool ci_scroll(arg_t a)
{
	direction_t dir = (direction_t) a;

	return img_pan(&img, dir, prefix);
}

bool ci_scroll_to_edge(arg_t a)
{
	direction_t dir = (direction_t) a;

	return img_pan_edge(&img, dir);
}

/* Xlib helper function for i_drag() */
Bool is_motionnotify(Display *d, XEvent *e, XPointer a)
{
	return e != NULL && e->type == MotionNotify;
}

#define WARP(x,y) \
	XWarpPointer(win.env.dpy, None, win.xwin, 0, 0, 0, 0, x, y); \
	ox = x, oy = y; \
	break

bool ci_drag(arg_t a)
{
	int dx = 0, dy = 0, i, ox, oy, x, y;
	unsigned int ui;
	bool dragging = true, next = false;
	XEvent e;
	Window w;

	if (!XQueryPointer(win.env.dpy, win.xwin, &w, &w, &i, &i, &ox, &oy, &ui))
		return false;
	
	win_set_cursor(&win, CURSOR_HAND);

	while (dragging) {
		if (!next)
			XMaskEvent(win.env.dpy,
			           ButtonPressMask | ButtonReleaseMask | PointerMotionMask, &e);
		switch (e.type) {
			case ButtonPress:
			case ButtonRelease:
				dragging = false;
				break;
			case MotionNotify:
				x = e.xmotion.x;
				y = e.xmotion.y;

				/* wrap the mouse around */
				if (x <= 0) {
					WARP(win.w - 2, y);
				} else if (x >= win.w - 1) {
					WARP(1, y);
				} else if (y <= 0) {
					WARP(x, win.h - 2);
				} else if (y >= win.h - 1) {
					WARP(x, 1);
				}
				dx += x - ox;
				dy += y - oy;
				ox = x;
				oy = y;
				break;
		}
		if (dragging)
			next = XCheckIfEvent(win.env.dpy, &e, is_motionnotify, None);
		if ((!dragging || !next) && (dx != 0 || dy != 0)) {
			if (img_move(&img, dx, dy)) {
				img_render(&img);
				win_draw(&win);
			}
			dx = dy = 0;
		}
	}
	win_set_cursor(&win, CURSOR_ARROW);
	set_timeout(reset_cursor, TO_CURSOR_HIDE, true);
	reset_timeout(redraw);

	return true;
}

bool ci_zoom(arg_t a)
{
	long scale = (long) a;

	if (scale > 0)
		return img_zoom_in(&img);
	else if (scale < 0)
		return img_zoom_out(&img);
	else
		return false;
}

bool ci_set_zoom(arg_t a)
{
	return img_zoom(&img, (prefix ? prefix : (long) a) / 100.0);
}

bool ci_fit_to_win(arg_t a)
{
	scalemode_t sm = (scalemode_t) a;

	return img_fit_win(&img, sm);
}

bool ci_rotate(arg_t a)
{
	degree_t degree = (degree_t) a;

	img_rotate(&img, degree);
	return true;
}

bool ci_flip(arg_t a)
{
	flipdir_t dir = (flipdir_t) a;

	img_flip(&img, dir);
	return true;
}

bool ci_change_gamma(arg_t a)
{
	return img_change_gamma(&img, (long) a);
}

bool ci_toggle_antialias(arg_t a)
{
	img_toggle_antialias(&img);
	return true;
}

bool ci_toggle_alpha(arg_t a)
{
	img.alpha = !img.alpha;
	img.dirty = true;
	return true;
}

bool ci_slideshow(arg_t a)
{
	if (prefix > 0) {
		img.ss.on = true;
		img.ss.delay = prefix;
		set_timeout(slideshow, img.ss.delay * 1000, true);
	} else if (img.ss.on) {
		img.ss.on = false;
		reset_timeout(slideshow);
	} else {
		img.ss.on = true;
	}
	return true;
}

bool ct_move_sel(arg_t a)
{
	direction_t dir = (direction_t) a;

	return tns_move_selection(&tns, dir, prefix);
}

bool ct_reload_all(arg_t a)
{
	tns_free(&tns);
	tns_init(&tns, files, filecnt, &fileidx, &win);
	return false;
}


#undef  G_CMD
#define G_CMD(c) { -1, cg_##c },
#undef  I_CMD
#define I_CMD(c) { MODE_IMAGE, ci_##c },
#undef  T_CMD
#define T_CMD(c) { MODE_THUMB, ct_##c },

const cmd_t cmds[CMD_COUNT] = {
#include "commands.lst"
};

