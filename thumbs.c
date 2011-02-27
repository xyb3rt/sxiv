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

	if (!tns || !tns->thumbs)
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
	imlib_context_set_anti_alias(1);
	imlib_render_image_part_on_drawable_at_size(0, 0, w, h,
	                                            0, 0, t->w, t->h);
	tns->dirty = 1;

	if (im)
		imlib_free_image();
}

void tns_check_view(tns_t *tns, Bool scrolled) {
	int r;

	if (!tns)
		return;

	tns->first -= tns->first % tns->cols;
	r = tns->sel % tns->cols;

	if (scrolled) {
		/* move selection into visible area */
		if (tns->sel >= tns->first + tns->cols * tns->rows)
			tns->sel = tns->first + r + tns->cols * (tns->rows - 1);
		else if (tns->sel < tns->first)
			tns->sel = tns->first + r;
	} else {
		/* scroll to selection */
		if (tns->first + tns->cols * tns->rows <= tns->sel) {
			tns->first = tns->sel - r - tns->cols * (tns->rows - 1);
			tns->dirty = 1;
		} else if (tns->first > tns->sel) {
			tns->first = tns->sel - r;
			tns->dirty = 1;
		}
	}
}

void tns_render(tns_t *tns, win_t *win) {
	int i, cnt, r, x, y;
	thumb_t *t;

	if (!tns || !tns->dirty || !win)
		return;

	win_clear(win);

	tns->cols = MAX(1, win->w / thumb_dim);
	tns->rows = MAX(1, win->h / thumb_dim);

	if (tns->cnt < tns->cols * tns->rows) {
		tns->first = 0;
		cnt = tns->cnt;
	} else {
		tns_check_view(tns, False);
		cnt = tns->cols * tns->rows;
		if ((r = tns->first + cnt - tns->cnt) >= tns->cols)
			tns->first -= r - r % tns->cols;
		if (r > 0)
			cnt -= r % tns->cols;
	}

	r = cnt % tns->cols ? 1 : 0;
	tns->x = x = (win->w - MIN(cnt, tns->cols) * thumb_dim) / 2 + 5;
	tns->y = y = (win->h - (cnt / tns->cols + r) * thumb_dim) / 2 + 5;

	for (i = 0; i < cnt; ++i) {
		t = &tns->thumbs[tns->first + i];
		t->x = x + (THUMB_SIZE - t->w) / 2;
		t->y = y + (THUMB_SIZE - t->h) / 2;
		win_draw_pixmap(win, t->pm, t->x, t->y, t->w, t->h);
		if ((i + 1) % tns->cols == 0) {
			x = tns->x;
			y += thumb_dim;
		} else {
			x += thumb_dim;
		}
	}

	tns->dirty = 0;
	tns_highlight(tns, win, tns->sel, True);
}

void tns_highlight(tns_t *tns, win_t *win, int n, Bool hl) {
	thumb_t *t;

	if (!tns || !win)
		return;

	if (n >= 0 && n < tns->cnt) {
		t = &tns->thumbs[n];
		win_draw_rect(win, t->x - 2, t->y - 2, t->w + 4, t->h + 4, hl);
	}

	win_draw(win);
}

int tns_move_selection(tns_t *tns, win_t *win, tnsdir_t dir) {
	int old;

	if (!tns || !win)
		return 0;

	old = tns->sel;

	switch (dir) {
		case TNS_LEFT:
			if (tns->sel > 0)
				--tns->sel;
			break;
		case TNS_RIGHT:
			if (tns->sel < tns->cnt - 1)
				++tns->sel;
			break;
		case TNS_UP:
			if (tns->sel >= tns->cols)
				tns->sel -= tns->cols;
			break;
		case TNS_DOWN:
			if (tns->sel + tns->cols < tns->cnt)
				tns->sel += tns->cols;
			break;
	}

	if (tns->sel != old) {
		tns_highlight(tns, win, old, False);
		tns_check_view(tns, False);
		if (!tns->dirty)
			tns_highlight(tns, win, tns->sel, True);
	}

	return tns->sel != old;
}

int tns_scroll(tns_t *tns, tnsdir_t dir) {
	int old;

	if (!tns)
		return 0;

	old = tns->first;

	if (dir == TNS_DOWN && tns->first + tns->cols * tns->rows < tns->cnt) {
		tns->first += tns->cols;
		tns_check_view(tns, True);
		tns->dirty = 1;
	} else if (dir == TNS_UP && tns->first >= tns->cols) {
		tns->first -= tns->cols;
		tns_check_view(tns, True);
		tns->dirty = 1;
	}

	return tns->first != old;
}

int tns_translate(tns_t *tns, int x, int y) {
	int n;
	thumb_t *t;

	if (!tns || x < tns->x || y < tns->y)
		return -1;

	n = tns->first + (y - tns->y) / thumb_dim * tns->cols +
	    (x - tns->x) / thumb_dim;

	if (n < tns->cnt) {
		t = &tns->thumbs[n];
		if (x >= t->x && x <= t->x + t->w && y >= t->y && y <= t->y + t->h)
			return n;
	}

	return -1;
}
