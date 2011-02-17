/* sxiv: thumbs.c
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
#include <string.h>

#include <Imlib2.h>

#include "config.h"
#include "thumbs.h"
#include "util.h"

extern Imlib_Image *im_broken;
const int thumb_dim = THUMB_SIZE + 10;

void tns_init(tns_t *tns, int cnt) {
	if (!tns)
		return;

	tns->cnt = tns->first = tns->sel = 0;
	tns->thumbs = (thumb_t*) s_malloc(cnt * sizeof(thumb_t));
}

void tns_free(tns_t *tns, win_t *win) {
	int i;

	if (!tns)
		return;

	for (i = 0; i < tns->cnt; ++i)
		win_free_pixmap(win, tns->thumbs[i].pm);

	free(tns->thumbs);
	tns->thumbs = NULL;
}

void tns_load(tns_t *tns, win_t *win, const char *filename) {
	int w, h;
	float z, zw, zh;
	thumb_t *t;
	Imlib_Image *im;

	if (!tns || !win || !filename)
		return;

	if ((im = imlib_load_image(filename)))
		imlib_context_set_image(im);
	else
		imlib_context_set_image(im_broken);

	w = imlib_image_get_width();
	h = imlib_image_get_height();
	zw = (float) THUMB_SIZE / (float) w;
	zh = (float) THUMB_SIZE / (float) h;
	z = MIN(zw, zh);
	if (!im && z > 1.0)
		z = 1.0;

	t = &tns->thumbs[tns->cnt++];
	t->w = z * w;
	t->h = z * h;

	t->pm = win_create_pixmap(win, t->w, t->h);
	imlib_context_set_drawable(t->pm);
	imlib_render_image_part_on_drawable_at_size(0, 0, w, h,
	                                            0, 0, t->w, t->h);
	imlib_free_image();
}

void tns_render(tns_t *tns, win_t *win) {
	int i, cnt, x, y;

	if (!tns || !win)
		return;

	tns->cols = win->w / thumb_dim;
	tns->rows = win->h / thumb_dim;

	cnt = tns->cols * tns->rows;
	if (tns->first && tns->first + cnt > tns->cnt)
		tns->first = MAX(0, tns->cnt - cnt);
	cnt = MIN(tns->first + cnt, tns->cnt);

	win_clear(win);

	x = y = 5;
	i = tns->first;

	while (i < cnt) {
		tns->thumbs[i].x = x + (THUMB_SIZE - tns->thumbs[i].w) / 2;
		tns->thumbs[i].y = y + (THUMB_SIZE - tns->thumbs[i].h) / 2;
		win_draw_pixmap(win, tns->thumbs[i].pm, tns->thumbs[i].x,
		                tns->thumbs[i].y, tns->thumbs[i].w, tns->thumbs[i].h);
		if (++i % tns->cols == 0) {
			x = 5;
			y += thumb_dim;
		} else {
			x += thumb_dim;
		}
	}

	win_draw(win);
}

