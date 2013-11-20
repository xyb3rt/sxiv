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
extern int markcnt;
extern int alternate;

bool it_quit()
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

bool it_switch_mode()
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

bool it_toggle_fullscreen()
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

bool it_toggle_bar()
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

bool t_reload_all()
{
	if (mode == MODE_THUMB) {
		tns_free(&tns);
		tns_init(&tns, filecnt, &win);
		return true;
	} else {
		return false;
	}
}

bool it_reload_image()
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

bool it_remove_image()
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

bool i_navigate(long n)
{
  n += fileidx;
  if (n < 0) n = 0;
  if (n >= filecnt) n = filecnt - 1;
  if (mode == MODE_THUMB) {
    tns.sel = n;
    tns.dirty = true;
  }
  if (n != fileidx) {
    load_image(n);
    return true;
  }
  return false;
}

bool i_alternate()
{
	if (mode == MODE_IMAGE) {
		load_image(alternate);
		return true;
	} else {
		return false;
	}
}

bool it_first()
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

bool it_n_or_last(int a)
{
	int n = a != 0 && a - 1 < filecnt ? a - 1 : filecnt - 1;

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

bool i_navigate_frame(long a)
{
	if (mode == MODE_IMAGE && !img.multi.animate)
		return img_frame_navigate(&img, a);
	else
		return false;
}

bool i_toggle_animation()
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

bool it_scroll_move(direction_t dir, int n)
{
	if (mode == MODE_IMAGE)
		return img_pan(&img, dir, n);
	else
		return tns_move_selection(&tns, dir, n);
}

bool it_scroll_screen(direction_t dir)
{
	if (mode == MODE_IMAGE)
		return img_pan(&img, dir, -1);
	else
		return tns_scroll(&tns, dir, true);
}

bool i_scroll_to_edge(direction_t dir)
{
	if (mode == MODE_IMAGE)
		return img_pan_edge(&img, dir);
	else
		return false;
}

bool i_zoom(long scale)
{
	if (mode != MODE_IMAGE)
		return false;

	if (scale > 0)
		return img_zoom_in(&img);
	else if (scale < 0)
		return img_zoom_out(&img);
	else
		return false;
}

bool i_set_zoom(long scale)
{
	if (mode == MODE_IMAGE)
		return img_zoom(&img, scale / 100.0);
	else
		return false;
}

bool i_fit_to_win(scalemode_t sm)
{
	bool ret = false;

	if (mode == MODE_IMAGE) {
		if ((ret = img_fit_win(&img, sm)))
			img_center(&img);
	}
	return ret;
}

bool i_fit_to_img()
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

bool i_rotate(degree_t degree)
{
	if (mode == MODE_IMAGE) {
		img_rotate(&img, degree);
		return true;
	}	else {
		return false;
	}
}

bool i_flip(flipdir_t dir)
{
	if (mode == MODE_IMAGE) {
		img_flip(&img, dir);
		return true;
	} else {
		return false;
	}
}

bool i_toggle_antialias()
{
	if (mode == MODE_IMAGE) {
		img_toggle_antialias(&img);
		return true;
	} else {
		return false;
	}
}

bool it_toggle_alpha()
{
	img.alpha = tns.alpha = !img.alpha;
	if (mode == MODE_IMAGE)
		img.dirty = true;
	else
		tns.dirty = true;
	return true;
}

bool p_set_bar_left(char *str) {
  strncpy(win.bar.l, str, BAR_L_LEN);
  free(str);
  win.bar.skip_update = 1;
  return true;
}

bool p_set_bar_right(char *str) {
  strncpy(win.bar.r, str, BAR_R_LEN);
  free(str);
  win.bar.skip_update = 1;
  return true;
}

bool it_add_image(char *filename) {
  fileidx = filecnt;
  check_add_file(filename);
  free(filename);
  
  filecnt = fileidx;
  tns_free(&tns);
  tns_init(&tns, filecnt, &win);
  it_n_or_last(fileidx);
  if (mode == MODE_THUMB) fileidx--;
  return true;
}

int p_get_file_count(){
  return filecnt;
}

int p_get_file_index(){
  return fileidx;
}

bool it_redraw(){
  redraw(); // hangs on imlib_render_image_part_on_drawable_at_size
  return true;
}
