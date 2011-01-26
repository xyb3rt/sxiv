/* sxiv: image.h
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

#ifndef IMAGE_H
#define IMAGE_H

#include "window.h"

enum scalemode {
	SCALE_DOWN = 0,
	SCALE_FIT,
	SCALE_ZOOM
};

typedef enum pandir_e {
	PAN_LEFT = 0,
	PAN_RIGHT,
	PAN_UP,
	PAN_DOWN
} pandir_t;

typedef struct img_s {
	float zoom;
	unsigned char re;
	unsigned char checkpan;
	unsigned char zoomed;
	unsigned char aa;
	int x;
	int y;
	int w;
	int h;
} img_t;

void img_init(img_t*, win_t*);
void img_free(img_t*);

int img_load(img_t*, const char*);
void img_render(img_t*, win_t*);

int img_zoom_in(img_t*);
int img_zoom_out(img_t*);

int img_pan(img_t*, win_t*, pandir_t);

int img_rotate_left(img_t*, win_t*);
int img_rotate_right(img_t*, win_t*);

int img_toggle_antialias(img_t*);

#endif /* IMAGE_H */
