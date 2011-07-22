/* sxiv: thumbs.h
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

#ifndef THUMBS_H
#define THUMBS_H

#include <Imlib2.h>

#include "types.h"
#include "window.h"

typedef struct {
	Imlib_Image *im;
	const char *filename;
	int x;
	int y;
	int w;
	int h;
} thumb_t;

typedef struct {
	thumb_t *thumbs;
	int cap;
	int cnt;
	int x;
	int y;
	int cols;
	int rows;
	int first;
	int sel;
	unsigned char dirty;
} tns_t;

void tns_clean_cache(tns_t*);

void tns_init(tns_t*, int);
void tns_free(tns_t*);

int tns_load(tns_t*, int, const char*, unsigned char);

void tns_render(tns_t*, win_t*);
void tns_highlight(tns_t*, win_t*, int, Bool);

int tns_move_selection(tns_t*, win_t*, direction_t);
int tns_scroll(tns_t*, direction_t);

int tns_translate(tns_t*, int, int);

#endif /* THUMBS_H */
