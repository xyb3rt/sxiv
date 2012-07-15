/* sxiv: thumbs.h
 * Copyright (c) 2012 Bert Muennich <be.muennich at googlemail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef THUMBS_H
#define THUMBS_H

#include <X11/Xlib.h>
#include <Imlib2.h>

#include "types.h"
#include "window.h"

typedef struct {
	const fileinfo_t *file;
	Imlib_Image *im;
	int w;
	int h;
	int x;
	int y;
} thumb_t;

typedef struct {
	thumb_t *thumbs;
	int cap;
	int cnt;
	int first;
	int sel;

	win_t *win;
	int x;
	int y;
	int cols;
	int rows;

	bool alpha;
	bool dirty;
} tns_t;

void tns_clean_cache(tns_t*);

void tns_init(tns_t*, int, win_t*);
void tns_free(tns_t*);

bool tns_load(tns_t*, int, const fileinfo_t*, bool, bool);

void tns_render(tns_t*);
void tns_highlight(tns_t*, int, bool);

bool tns_move_selection(tns_t*, direction_t, int);
bool tns_scroll(tns_t*, direction_t, bool);

int tns_translate(tns_t*, int, int);

#endif /* THUMBS_H */
