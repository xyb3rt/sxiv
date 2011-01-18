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

#include <Imlib2.h>

#include "window.h"

typedef enum scalemode_e {
	SCALE_DOWN = 0,
	SCALE_FIT,
	SCALE_ZOOM
} scalemode_t;

typedef struct img_s {
	int zoom;
	scalemode_t scalemode;
	int w;
	int h;
	int x;
	int y;

	Imlib_Image *im;
} img_t;

void imlib_init(win_t*);

void img_load(img_t*, char*);
void img_display(img_t*, win_t*);

#endif /* IMAGE_H */
