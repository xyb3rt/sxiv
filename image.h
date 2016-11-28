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

#ifndef IMAGE_H
#define IMAGE_H

#include <Imlib2.h>

#include "types.h"
#include "window.h"

typedef struct {
	Imlib_Image im;
	unsigned int delay;
} img_frame_t;

typedef struct {
	img_frame_t *frames;
	int cap;
	int cnt;
	int sel;
	bool animate;
	int framedelay;
	int length;
} multi_img_t;

typedef struct {
	Imlib_Image im;
	int w;
	int h;

	win_t *win;
	float x;
	float y;

	scalemode_t scalemode;
	float zoom;

	bool checkpan;
	bool dirty;
	bool aa;
	bool alpha;

	Imlib_Color_Modifier cmod;
	int gamma;

	struct {
		bool on;
		int delay;
	} ss;

	multi_img_t multi;
} img_t;

void img_init(img_t*, win_t*);

bool img_load(img_t*, const fileinfo_t*);
CLEANUP void img_close(img_t*, bool);

void img_render(img_t*);

bool img_fit_win(img_t*, scalemode_t);

bool img_zoom(img_t*, float);
bool img_zoom_in(img_t*);
bool img_zoom_out(img_t*);

bool img_move(img_t*, float, float);
bool img_pan(img_t*, direction_t, int);
bool img_pan_edge(img_t*, direction_t);

void img_rotate(img_t*, degree_t);
void img_flip(img_t*, flipdir_t);

void img_toggle_antialias(img_t*);

bool img_change_gamma(img_t*, int);

bool img_frame_navigate(img_t*, int);
bool img_frame_animate(img_t*);

#endif /* IMAGE_H */
