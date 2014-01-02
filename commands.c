/* Copyright 2011, 2012 Bert Muennich
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
void set_timeout(timeout_f, int, bool);
void reset_timeout(timeout_f);

extern appmode_t mode;
extern img_t img;
extern tns_t tns;
extern win_t win;

extern fileinfo_t *files;
extern int filecnt, fileidx;
extern int alternate;

extern int prefix;

const int ss_delays[] = {
	1, 2, 3, 5, 10, 15, 20, 30, 60, 120, 180, 300, 600
};

bool it_quit(arg_t a)
{
	unsigned int i;

	if (options->to_stdout) {
		for (i = 0; i < filecnt; i++) {
			if (files[i].marked)
				printf("%s\n", files[i].name);
		}
	}
	cleanup();
	exit(EXIT_SUCCESS);
}

bool it_switch_mode(arg_t a)
{
	if (mode == MODE_IMAGE) {
		if (tns.thumbs == NULL) {
			tns_init(&tns, filecnt, &win);
			tns.alpha = img.alpha;
		}
		img_close(&img, false);
		reset_timeout(reset_cursor);
		tns.sel = fileidx;
		tns.dirty = true;
		mode = MODE_THUMB;
	} else {
		load_image(tns.sel);
		mode = MODE_IMAGE;
	}
	return true;
}

bool it_toggle_fullscreen(arg_t a)
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

bool it_toggle_bar(arg_t a)
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

bool t_reload_all(arg_t a)
{
	if (mode == MODE_THUMB) {
		tns_free(&tns);
		tns_init(&tns, filecnt, &win);
		return true;
	} else {
		return false;
	}
}

bool it_reload_image(arg_t a)
{
	if (mode == MODE_IMAGE) {
		load_image(fileidx);
	} else {
		win_set_cursor(&win, CURSOR_WATCH);
		if (!tns_load(&tns, tns.sel, &files[tns.sel], true, false)) {
			remove_file(tns.sel, false);
			tns.dirty = true;
			if (tns.sel >= tns.cnt)
				tns.sel = tns.cnt - 1;
		}
	}
	return true;
}

bool it_remove_image(arg_t a)
{
	if (mode == MODE_IMAGE) {
		remove_file(fileidx, true);
		load_image(fileidx >= filecnt ? filecnt - 1 : fileidx);
		return true;
	} else if (tns.sel < tns.cnt) {
		remove_file(tns.sel, true);
		tns.dirty = true;
		if (tns.sel >= tns.cnt)
			tns.sel = tns.cnt - 1;
		return true;
	} else {
		return false;
	}
}

bool i_navigate(arg_t a)
{
	long n = (long) a;

	if (mode == MODE_IMAGE) {
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
		}
	}
	return false;
}

bool i_alternate(arg_t a)
{
	if (mode == MODE_IMAGE) {
		load_image(alternate);
		return true;
	} else {
		return false;
	}
}

bool it_first(arg_t a)
{
	if (mode == MODE_IMAGE && fileidx != 0) {
		load_image(0);
		return true;
	} else if (mode == MODE_THUMB && tns.sel != 0) {
		tns.sel = 0;
		tns.dirty = true;
		return true;
	} else {
		return false;
	}
}

bool it_n_or_last(arg_t a)
{
	int n = prefix != 0 && prefix - 1 < filecnt ? prefix - 1 : filecnt - 1;

	if (mode == MODE_IMAGE && fileidx != n) {
		load_image(n);
		return true;
	} else if (mode == MODE_THUMB && tns.sel != n) {
		tns.sel = n;
		tns.dirty = true;
		return true;
	} else {
		return false;
	}
}

bool i_navigate_frame(arg_t a)
{
	if (mode == MODE_IMAGE && !img.multi.animate)
		return img_frame_navigate(&img, (long) a);
	else
		return false;
}

bool i_toggle_animation(arg_t a)
{
	if (mode != MODE_IMAGE)
		return false;

	if (img.multi.animate) {
		reset_timeout(animate);
		img.multi.animate = false;
	} else if (img_frame_animate(&img, true)) {
		set_timeout(animate, img.multi.frames[img.multi.sel].delay, true);
	}
	return true;
}

bool it_toggle_image_mark(arg_t a)
{
	int sel = mode == MODE_IMAGE ? fileidx : tns.sel;

	files[sel].marked = !files[sel].marked;
	if (mode == MODE_THUMB)
		tns_mark(&tns, sel, files[sel].marked);
	return true;
}

bool it_reverse_marks(arg_t a)
{
	int i, cnt = mode == MODE_IMAGE ? filecnt : tns.cnt;

	for (i = 0; i < cnt; i++)
		files[i].marked = !files[i].marked;
	if (mode == MODE_THUMB)
		tns.dirty = true;
	return true;
}

bool it_navigate_marked(arg_t a)
{
	long n = (long) a;
	int d, i, cnt, sel, new;
	
	if (mode == MODE_IMAGE)
		cnt = filecnt, sel = new = fileidx;
	else
		cnt = tns.cnt, sel = new = tns.sel;
	if (prefix > 0)
		n *= prefix;
	d = n > 0 ? 1 : -1;
	for (i = sel + d; n != 0 && i >= 0 && i < cnt; i += d) {
		if (files[i].marked) {
			n -= d;
			new = i;
		}
	}
	if (new != sel) {
		if (mode == MODE_IMAGE) {
			load_image(new);
		} else {
			tns.sel = new;
			tns.dirty = true;
		}
		return true;
	} else {
		return false;
	}
}

bool it_scroll_move(arg_t a)
{
	direction_t dir = (direction_t) a;

	if (mode == MODE_IMAGE)
		return img_pan(&img, dir, prefix);
	else
		return tns_move_selection(&tns, dir, prefix);
}

bool it_scroll_screen(arg_t a)
{
	direction_t dir = (direction_t) a;

	if (mode == MODE_IMAGE)
		return img_pan(&img, dir, -1);
	else
		return tns_scroll(&tns, dir, true);
}

bool i_scroll_to_edge(arg_t a)
{
	direction_t dir = (direction_t) a;

	if (mode == MODE_IMAGE)
		return img_pan_edge(&img, dir);
	else
		return false;
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

bool i_drag(arg_t a)
{
	int dx = 0, dy = 0, i, ox, oy, x, y;
	unsigned int ui;
	bool dragging = true, next = false;
	XEvent e;
	Window w;

	if (mode != MODE_IMAGE)
		return false;
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

	return false;
}

bool i_zoom(arg_t a)
{
	long scale = (long) a;

	if (mode != MODE_IMAGE)
		return false;

	if (scale > 0)
		return img_zoom_in(&img);
	else if (scale < 0)
		return img_zoom_out(&img);
	else
		return false;
}

bool i_set_zoom(arg_t a)
{
	if (mode == MODE_IMAGE)
		return img_zoom(&img, (prefix ? prefix : (long) a) / 100.0);
	else
		return false;
}

bool i_fit_to_win(arg_t a)
{
	bool ret = false;
	scalemode_t sm = (scalemode_t) a;

	if (mode == MODE_IMAGE) {
		if ((ret = img_fit_win(&img, sm)))
			img_center(&img);
	}
	return ret;
}

bool i_fit_to_img(arg_t a)
{
	int x, y;
	unsigned int w, h;
	bool ret = false;

	if (mode == MODE_IMAGE) {
		x = MAX(0, win.x + img.x);
		y = MAX(0, win.y + img.y);
		w = img.w * img.zoom;
		h = img.h * img.zoom;
		if ((ret = win_moveresize(&win, x, y, w, h))) {
			img.x = x - win.x;
			img.y = y - win.y;
			img.dirty = true;
		}
	}
	return ret;
}

bool i_rotate(arg_t a)
{
	degree_t degree = (degree_t) a;

	if (mode == MODE_IMAGE) {
		img_rotate(&img, degree);
		return true;
	}	else {
		return false;
	}
}

bool i_flip(arg_t a)
{
	flipdir_t dir = (flipdir_t) a;

	if (mode == MODE_IMAGE) {
		img_flip(&img, dir);
		return true;
	} else {
		return false;
	}
}

bool i_toggle_antialias(arg_t a)
{
	if (mode == MODE_IMAGE) {
		img_toggle_antialias(&img);
		return true;
	} else {
		return false;
	}
}

bool i_change_gamma(arg_t a)
{
	if (mode == MODE_IMAGE) {
		return img_change_gamma(&img, (long) a);
	} else {
		return false;
	}
}

bool it_toggle_alpha(arg_t a)
{
	img.alpha = tns.alpha = !img.alpha;
	if (mode == MODE_IMAGE)
		img.dirty = true;
	else
		tns.dirty = true;
	return true;
}

