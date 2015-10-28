/* Copyright 2011 Bert Muennich
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

#ifndef THUMBS_H
#define THUMBS_H

#include <X11/Xlib.h>
#include <Imlib2.h>

#include "types.h"
#include "window.h"

typedef struct {
	Imlib_Image im;
	int w;
	int h;
	int x;
	int y;
} thumb_t;

typedef struct {
	fileinfo_t *files;
	thumb_t *thumbs;
	const int *cnt;
	int *sel;
	int initnext;
	int loadnext;
	int first, end;
	int r_first, r_end;

	win_t *win;
	int x;
	int y;
	int cols;
	int rows;
	int zl;
	int bw;
	int dim;

	bool dirty;
} tns_t;

void tns_clean_cache(tns_t*);

void tns_init(tns_t*, fileinfo_t*, const int*, int*, win_t*);
CLEANUP void tns_free(tns_t*);

bool tns_load(tns_t*, int, bool, bool);
void tns_unload(tns_t*, int);

void tns_render(tns_t*);
void tns_mark(tns_t*, int, bool);
void tns_highlight(tns_t*, int, bool);

bool tns_move_selection(tns_t*, direction_t, int);
bool tns_scroll(tns_t*, direction_t, bool);

bool tns_zoom(tns_t*, int);

int tns_translate(tns_t*, int, int);

#endif /* THUMBS_H */
