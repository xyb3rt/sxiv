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

#ifndef OPTIONS_H
#define OPTIONS_H

#include "types.h"
#include "image.h"

typedef struct {
	/* file list: */
	char **filenames;
	bool from_stdin;
	bool to_stdout;
	bool recursive;
	int filecnt;
	int startnum;

	/* image: */
	scalemode_t scalemode;
	float zoom;
	bool animate;
	int gamma;
	int slideshow;
	int framerate;

	/* window: */
	bool fullscreen;
	bool hide_bar;
	long embed;
	char *geometry;
	char *res_name;

	/* misc flags: */
	bool quiet;
	bool thumb_mode;
	bool clean_cache;
} options_t;

extern const options_t *options;

void print_usage(void);
void print_version(void);

void parse_options(int, char**);

#endif /* OPTIONS_H */
