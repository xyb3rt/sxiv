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

#include "sxiv.h"
#include "image.h"

void imlib_init(win_t *win) {
	if (!win)
		return;

	imlib_context_set_display(win->env.dpy);
	imlib_context_set_visual(win->env.vis);
	imlib_context_set_colormap(win->env.cmap);
	imlib_context_set_drawable(win->xwin);
}

void img_load(img_t *img, char *filename) {
	if (!img || !filename)
		return;

	if (imlib_context_get_image())
		imlib_free_image();

	if (!(img->im = imlib_load_image(filename)))
		DIE("could not open image: %s", filename);

	imlib_context_set_image(img->im);

	img->w = imlib_image_get_width();
	img->h = imlib_image_get_height();
}

void img_render(img_t *img, win_t *win) {
	float zw, zh;
	unsigned int sx, sy, sw, sh;
	unsigned int dx, dy, dw, dh;

	if (!img || !win)
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

	if (img->x < 0) {
		sx = -img->x / img->zoom;
		sw = (img->x + win->w) / img->zoom;
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
		sh = (img->y + win->h) / img->zoom;
		dy = 0;
		dh = win->h;
	} else {
		sy = 0;
		sh = img->h;
		dy = img->y;
		dh = img->h * img->zoom;
	}

	win_clear(win);

	imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw, dh);
}
