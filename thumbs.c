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
#include <pthread.h>

#include <Imlib2.h>

#include "config.h"
#include "thumbs.h"
#include "util.h"

typedef struct tload_s {
	const char **filenames;
	tns_t *tns;
	win_t *win;
} tload_t;

const int thumb_dim = THUMB_SIZE + 10;

pthread_t loader;
tload_t tinfo;

void* thread_load(void *arg) {
	int i, w, h;
	float z, zw, zh;
	tload_t *tl;
	thumb_t *t;
	Imlib_Image *im;

	tl = (tload_t*) arg;

	for (i = 0; i < tl->tns->cnt; ++i) {
		if (!(im = imlib_load_image(tl->filenames[i])))
			continue;

		imlib_context_set_image(im);

		w = imlib_image_get_width();
		h = imlib_image_get_height();
		zw = (float) THUMB_SIZE / (float) w;
		zh = (float) THUMB_SIZE / (float) h;
		z = MIN(zw, zh);

		t = &tl->tns->thumbs[i];
		t->w = z * w;
		t->h = z * h;

		t->pm = win_create_pixmap(tl->win, t->w, t->h);
		imlib_context_set_drawable(t->pm);
		imlib_render_image_part_on_drawable_at_size(0, 0, w, h,
		                                            0, 0, t->w, t->h);
		t->loaded = 1;
		imlib_free_image();
	}

	tl->tns->loaded = 1;

	return 0;
}

void tns_load(tns_t *tns, win_t *win, const char **fnames, int fcnt) {
	pthread_attr_t tattr;

	if (!tns || !win || !fnames || !fcnt)
		return;
	
	tns->thumbs = (thumb_t*) s_malloc(fcnt * sizeof(thumb_t));
	memset(tns->thumbs, 0, fcnt * sizeof(thumb_t));
	tns->cnt = fcnt;
	tns->loaded = 0;

	tinfo.filenames = fnames;
	tinfo.tns = tns;
	tinfo.win = win;

	pthread_attr_init(&tattr);
	pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_JOINABLE);

	if (pthread_create(&loader, &tattr, thread_load, (void*) &tinfo))
		die("could not create thread");
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
		if (!tns->thumbs[i].loaded)
			continue;

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

