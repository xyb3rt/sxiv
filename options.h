/* sxiv: options.h
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

#ifndef OPTIONS_H
#define OPTIONS_H

#include "image.h"
#include "types.h"

typedef struct {
	char **filenames;
	unsigned char from_stdin;
	int filecnt;
	int startnum;

	scalemode_t scalemode;
	float zoom;
	unsigned char aa;
	unsigned char thumbnails;

	unsigned char fixed;
	unsigned char fullscreen;
	char *geometry;

	unsigned char quiet;
	unsigned char clean_cache;
	unsigned char recursive;
} options_t;

extern const options_t *options;

void print_usage();
void print_version();

void parse_options(int, char**);

#endif /* OPTIONS_H */
