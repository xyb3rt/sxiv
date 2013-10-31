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

#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

typedef enum {
	BO_BIG_ENDIAN,
	BO_LITTLE_ENDIAN
} byteorder_t;

typedef enum {
	MODE_IMAGE,
	MODE_THUMB
} appmode_t;

typedef enum {
	DIR_LEFT,
	DIR_RIGHT,
	DIR_UP,
	DIR_DOWN
} direction_t;

typedef enum {
	DEGREE_90  = 1,
	DEGREE_180 = 2,
	DEGREE_270 = 3
} degree_t;

typedef enum {
	FLIP_HORIZONTAL,
	FLIP_VERTICAL
} flipdir_t;

typedef enum {
	SCALE_DOWN,
	SCALE_FIT,
	SCALE_WIDTH,
	SCALE_HEIGHT,
	SCALE_ZOOM
} scalemode_t;

typedef enum {
	CURSOR_ARROW,
	CURSOR_NONE,
	CURSOR_HAND,
	CURSOR_WATCH
} cursor_t;

typedef struct {
	const char *name; /* as given by user */
	const char *path; /* always absolute */
	const char *base;
	bool loaded;
	bool marked;
} fileinfo_t;

/* timeouts in milliseconds: */
enum {
	TO_REDRAW_RESIZE = 75,
	TO_REDRAW_THUMBS = 200,
	TO_CURSOR_HIDE   = 1200,
	TO_LOAD_NEXT     = 100,
};

typedef void (*timeout_f)(void);

#endif /* TYPES_H */
