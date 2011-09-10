/* sxiv: image.h
 * Copyright (c) 2011 Bert Muennich <muennich at informatik.hu-berlin.de>
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

#ifndef IMAGE_H
#define IMAGE_H

#include <Imlib2.h>

#include "types.h"
#include "window.h"

typedef struct {
	Imlib_Image *im;
	unsigned int delay;
} img_frame_t;

typedef struct {
	img_frame_t *frames;
	int cap;
	int cnt;
	int sel;
	unsigned char animate;
} multi_img_t;

typedef struct {
	Imlib_Image *im;
	multi_img_t multi;

	float zoom;
	scalemode_t scalemode;

	unsigned char re;
	unsigned char checkpan;
	unsigned char dirty;
	unsigned char aa;
	unsigned char alpha;

	unsigned char slideshow;
	int ss_delay; /* in ms */

	int x;
	int y;
	int w;
	int h;
} img_t;

void img_init(img_t*, win_t*);

int img_load(img_t*, const fileinfo_t*);
void img_close(img_t*, int);

void img_render(img_t*, win_t*);

int img_fit_win(img_t*, win_t*);
int img_center(img_t*, win_t*);

int img_zoom(img_t*, win_t*, float);
int img_zoom_in(img_t*, win_t*);
int img_zoom_out(img_t*, win_t*);

int img_move(img_t*, win_t*, int, int);
int img_pan(img_t*, win_t*, direction_t, int);
int img_pan_edge(img_t*, win_t*, direction_t);

void img_rotate_left(img_t*, win_t*);
void img_rotate_right(img_t*, win_t*);

void img_toggle_antialias(img_t*);

int img_frame_navigate(img_t*, int);
int img_frame_animate(img_t*, int);

#endif /* IMAGE_H */
