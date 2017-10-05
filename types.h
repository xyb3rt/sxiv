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

/*
 * Annotation for functions called in cleanup().
 * These functions are not allowed to call error(!0, ...) or exit().
 */
#define CLEANUP

typedef enum {
	BO_BIG_ENDIAN,
	BO_LITTLE_ENDIAN
} byteorder_t;

typedef enum {
	MODE_IMAGE,
	MODE_THUMB
} appmode_t;

typedef enum {
	DIR_LEFT  = 1,
	DIR_RIGHT = 2,
	DIR_UP    = 4,
	DIR_DOWN  = 8
} direction_t;

typedef enum {
	DEGREE_90  = 1,
	DEGREE_180 = 2,
	DEGREE_270 = 3
} degree_t;

typedef enum {
	FLIP_HORIZONTAL = 1,
	FLIP_VERTICAL   = 2
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
	CURSOR_DRAG,
	CURSOR_WATCH,
	CURSOR_NONE,

	CURSOR_COUNT
} cursor_t;

typedef enum {
	FF_WARN    = 1,
	FF_MARK    = 2,
	FF_TN_INIT = 4
} fileflags_t;

typedef struct {
	const char *name; /* as given by user */
	const char *path; /* always absolute */
	const char *base;
	fileflags_t flags;
} fileinfo_t;

/* timeouts in milliseconds: */
enum {
	TO_REDRAW_RESIZE = 75,
	TO_REDRAW_THUMBS = 200,
	TO_CURSOR_HIDE   = 1200,
	TO_DOUBLE_CLICK  = 300
};

typedef void (*timeout_f)(void);

#endif /* TYPES_H */
