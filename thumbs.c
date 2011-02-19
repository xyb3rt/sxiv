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
	memset(tns->thumbs, 0, cnt * sizeof(thumb_t));
	tns->dirty = 0;
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

	tns->dirty = 1;
}

void tns_render(tns_t *tns, win_t *win) {
	int i, cnt, x, y;

	if (!tns || !tns->dirty || !win)
		return;

	tns->cols = MAX(1, win->w / thumb_dim);
	tns->rows = MAX(1, win->h / thumb_dim);

	cnt = tns->cols * tns->rows;
	if (tns->first && tns->first + cnt > tns->cnt)
		tns->first = MAX(0, tns->cnt - cnt);
	cnt = MIN(tns->first + cnt, tns->cnt);

	win_clear(win);

	i = cnt % tns->cols ? 1 : 0;
	tns->x = x = (win->w - MIN(cnt, tns->cols) * thumb_dim) / 2 + 5;
	tns->y = y = (win->h - (cnt / tns->cols + i) * thumb_dim) / 2 + 5;
	i = tns->first;

	while (i < cnt) {
		tns->thumbs[i].x = x + (THUMB_SIZE - tns->thumbs[i].w) / 2;
		tns->thumbs[i].y = y + (THUMB_SIZE - tns->thumbs[i].h) / 2;
		win_draw_pixmap(win, tns->thumbs[i].pm, tns->thumbs[i].x,
		                tns->thumbs[i].y, tns->thumbs[i].w, tns->thumbs[i].h);
		if (++i % tns->cols == 0) {
			x = tns->x;
			y += thumb_dim;
		} else {
			x += thumb_dim;
		}
	}

	tns->dirty = 0;

	tns_highlight(tns, win, -1);
}

void tns_highlight(tns_t *tns, win_t *win, int old) {
	thumb_t *t;

	if (!tns || !win)
		return;

	if (old >= 0 && old < tns->cnt) {
		t = &tns->thumbs[old];
		win_draw_rect(win, t->x - 2, t->y - 2, t->w + 4, t->h + 4, False);
	}
	if (tns->sel < tns->cnt) {
		t = &tns->thumbs[tns->sel];
		win_draw_rect(win, t->x - 2, t->y - 2, t->w + 4, t->h + 4, True);
	}

	win_draw(win);
}

int tns_move_selection(tns_t *tns, win_t *win, movedir_t dir) {
	int sel, old;

	if (!tns || !win)
		return 0;

	sel = old = tns->sel;

	switch (dir) {
		case MOVE_LEFT:
			if (sel % tns->cols > 0)
				--sel;
			break;
		case MOVE_RIGHT:
			if (sel % tns->cols < tns->cols - 1 && sel < tns->cnt - 1)
				++sel;
			break;
		case MOVE_UP:
			if (sel / tns->cols > 0)
				sel -= tns->cols;
			break;
		case MOVE_DOWN:
			if (sel / tns->cols < tns->rows - 1 && sel + tns->cols < tns->cnt)
				sel += tns->cols;
			break;
	}

	if (sel != old && tns->thumbs[sel].x != 0) {
		tns->sel = sel;
		tns_highlight(tns, win, old);
	}

	return sel != old;
}

int tns_translate(tns_t *tns, int x, int y) {
	int n;
	thumb_t *t;

	if (!tns || x < tns->x || y < tns->y)
		return -1;

	n = (y - tns->y) / thumb_dim * tns->cols + (x - tns->x) / thumb_dim;

	if (n < tns->cnt) {
		t = &tns->thumbs[n];
		if (x >= t->x && x <= t->x + t->w && y >= t->y && y <= t->y + t->h)
			return n;
	}

	return -1;
}
