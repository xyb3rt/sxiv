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

#include "window.h"

typedef struct thumb_s {
	int x;
	int y;
	int w;
	int h;
	Pixmap pm;
	unsigned char loaded;
} thumb_t;

typedef struct tns_s {
	thumb_t *thumbs;
	unsigned char loaded;
	int cnt;
	int cols;
	int rows;
	int first;
	int sel;
} tns_t;

extern const int thumb_dim;

void tns_load(tns_t*, win_t*, const char**, int);
void tns_free(tns_t*, win_t*);

void tns_render(tns_t*, win_t*);

#endif /* THUMBS_H */
