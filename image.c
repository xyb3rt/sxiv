/* sxiv: image.c
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

#include <Imlib2.h>

#include "sxiv.h"
#include "image.h"

int zl_cnt;
float zoom_min;
float zoom_max;

void img_init(img_t *img, win_t *win) {
	zl_cnt = sizeof(zoom_levels) / sizeof(zoom_levels[0]);
	zoom_min = zoom_levels[0] / 100.0;
	zoom_max = zoom_levels[zl_cnt - 1] / 100.0;

	if (img)
		img->zoom = 1.0;

	if (win) {
		imlib_context_set_display(win->env.dpy);
		imlib_context_set_visual(win->env.vis);
		imlib_context_set_colormap(win->env.cmap);
	}
}

void img_free(img_t* img) {
	if (imlib_context_get_image())
		imlib_free_image();
}

int img_load(img_t *img, const char *filename) {
	Imlib_Image *im;

	if (!img || !filename)
		return -1;

	if (imlib_context_get_image())
		imlib_free_image();

	if (!(im = imlib_load_image(filename))) {
		WARN("could not open image: %s", filename);
		return -1;
	}

	imlib_context_set_image(im);

	img->re = 0;
	img->w = imlib_image_get_width();
	img->h = imlib_image_get_height();

	return 0;
}

void img_check_pan(img_t *img, win_t *win) {
	if (!img)
		return;

	if (img->w * img->zoom > win->w) {
		if (img->x > 0 && img->x + img->w * img->zoom > win->w)
			img->x = 0;
		if (img->x < 0 && img->x + img->w * img->zoom < win->w)
			img->x = win->w - img->w * img->zoom;
	} else {
		img->x = (win->w - img->w * img->zoom) / 2;
	}
	if (img->h * img->zoom > win->h) {
		if (img->y > 0 && img->y + img->h * img->zoom > win->h)
			img->y = 0;
		if (img->y < 0 && img->y + img->h * img->zoom < win->h)
			img->y = win->h - img->h * img->zoom;
	} else {
		img->y = (win->h - img->h * img->zoom) / 2;
	}
}

void img_render(img_t *img, win_t *win) {
	int sx, sy, sw, sh;
	int dx, dy, dw, dh;
	float zw, zh;

	if (!img || !win || !imlib_context_get_image())
		return;

	if (!img->re) {
		/* rendered for the first time */
		img->re = 1;

		/* set zoom level to fit image into window */
		if (SCALE_MODE != SCALE_ZOOM) {
			zw = (float) win->w / (float) img->w;
			zh = (float) win->h / (float) img->h;
			img->zoom = MIN(zw, zh);

			if (img->zoom < zoom_min)
				img->zoom = zoom_min;
			else if (img->zoom > zoom_max)
				img->zoom = zoom_max;

			if (SCALE_MODE == SCALE_DOWN && img->zoom > 1.0)
				img->zoom = 1.0;
		}

		/* center image in window */
		img->x = (win->w - img->w * img->zoom) / 2;
		img->y = (win->h - img->h * img->zoom) / 2;
	} else if (img->cp) {
		/* only useful after zooming */
		img_check_pan(img, win);
		img->cp = 0;
	}

	/* calculate source and destination offsets */
	if (img->x < 0) {
		sx = -img->x / img->zoom;
		sw = win->w / img->zoom;
		dx = 0;
		dw = win->w;
	} else {
		sx = 0;
		sw = img->w;
		dx = img->x;
		dw = img->w * img->zoom;
	}
	if (img->y < 0) {
		sy = -img->y / img->zoom;
		sh = win->h / img->zoom;
		dy = 0;
		dh = win->h;
	} else {
		sy = 0;
		sh = img->h;
		dy = img->y;
		dh = img->h * img->zoom;
	}

	win_clear(win);

	imlib_context_set_drawable(win->pm);
	imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw, dh);

	win_draw(win);
}

int img_zoom(img_t *img, float z) {
	if (!img)
		return 0;

	if (z < zoom_min)
		z = zoom_min;
	else if (z > zoom_max)
		z = zoom_max;

	if (z != img->zoom) {
		img->x -= (img->w * z - img->w * img->zoom) / 2;
		img->y -= (img->h * z - img->h * img->zoom) / 2;
		img->zoom = z;
		img->cp = 1;
		return 1;
	} else {
		return 0;
	}
}

int img_zoom_in(img_t *img) {
	int i;

	if (!img)
		return 0;

	for (i = 1; i < zl_cnt; ++i) {
		if (zoom_levels[i] > img->zoom * 100.0)
			return img_zoom(img, zoom_levels[i] / 100.0);
	}

	return 0;
}

int img_zoom_out(img_t *img) {
	int i;

	if (!img)
		return 0;

	for (i = zl_cnt - 2; i >= 0; --i) {
		if (zoom_levels[i] < img->zoom * 100.0)
			return img_zoom(img, zoom_levels[i] / 100.0);
	}

	return 0;
}

int img_pan(img_t *img, win_t *win, pandir_t dir) {
	int ox, oy;

	if (!img || !win)
		return 0;

	ox = img->x;
	oy = img->y;

	switch (dir) {
		case PAN_LEFT:
			img->x += win->w / 5;
			break;
		case PAN_RIGHT:
			img->x -= win->w / 5;
			break;
		case PAN_UP:
			img->y += win->h / 5;
			break;
		case PAN_DOWN:
			img->y -= win->h / 5;
			break;
	}

	img_check_pan(img, win);

	return ox != img->x || oy != img->y;
}
