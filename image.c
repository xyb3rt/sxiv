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

void imlib_init(win_t *win) {
	if (!win)
		return;

	imlib_context_set_display(win->env.dpy);
	imlib_context_set_visual(win->env.vis);
	imlib_context_set_colormap(win->env.cmap);
}

void imlib_destroy() {
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

	img->w = imlib_image_get_width();
	img->h = imlib_image_get_height();

	return 0;
}

void img_display(img_t *img, win_t *win) {
	float zw, zh;

	if (!img || !win || !imlib_context_get_image())
		return;

	/* set zoom level to fit image into window */
	if (img->scalemode != SCALE_ZOOM) {
		zw = (float) win->w / (float) img->w;
		zh = (float) win->h / (float) img->h;
		img->zoom = MIN(zw, zh);

		if (img->zoom * 100.0 < ZOOM_MIN)
			img->zoom = ZOOM_MIN / 100.0;
		else if (img->zoom * 100.0 > ZOOM_MAX)
			img->zoom = ZOOM_MAX / 100.0;

		if (img->scalemode == SCALE_DOWN && img->zoom > 1.0)
			img->zoom = 1.0;
	}

	/* center image in window */
	img->x = (win->w - img->w * img->zoom) / 2;
	img->y = (win->h - img->h * img->zoom) / 2;

	img_render(img, win);
}

void img_render(img_t *img, win_t *win) {
	int sx, sy, sw, sh;
	int dx, dy, dw, dh;

	if (!img || !win || !imlib_context_get_image())
		return;

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
