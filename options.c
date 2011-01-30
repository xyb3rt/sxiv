/* sxiv: options.c
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

#define _XOPEN_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "sxiv.h"
#include "options.h"

options_t _options;
const options_t *options = (const options_t*) &_options;

void print_usage() {
	printf("usage: sxiv [-dfhpsvWZ] [-w WIDTH[xHEIGHT]] [-z ZOOM] FILES...\n");
}

void print_version() {
	printf("sxiv - simple x image viewer\n");
	printf("Version %s, written by Bert Muennich\n", VERSION);
}

void parse_options(int argc, char **argv) {
	unsigned short w, h;
	float z;
	int opt;

	_options.scalemode = SCALE_MODE;
	_options.zoom = 1.0;
	_options.aa = 1;

	_options.winw = w = 0;
	_options.winh = h = 0;
	_options.fullscreen = 0;

	_options.warn = 0;

	while ((opt = getopt(argc, argv, "dfhpsvWw:Zz:")) != -1) {
		switch (opt) {
			case '?':
				print_usage();
				exit(1);
			case 'd':
				_options.scalemode = SCALE_DOWN;
				break;
			case 'f':
				_options.fullscreen = 1;
				break;
			case 'h':
				print_usage();
				exit(0);
			case 'p':
				_options.aa = 0;
				break;
			case 's':
				_options.scalemode = SCALE_FIT;
				break;
			case 'v':
				print_version();
				exit(0);
			case 'W':
				_options.warn = 1;
				break;
			case 'w':
				if (!sscanf(optarg, "%hux%hu", &w, &h)) {
					fprintf(stderr, "sxiv: invalid argument for option -w: %s\n",
					        optarg);
					exit(1);
				} else {
					_options.winw = (int) w;
					_options.winh = (int) h;
				}
				break;
			case 'Z':
				_options.scalemode = SCALE_ZOOM;
				_options.zoom = 1.0;
				break;
			case 'z':
				_options.scalemode = SCALE_ZOOM;
				if (!sscanf(optarg, "%f", &z) || z < 0) {
					fprintf(stderr, "sxiv: invalid argument for option -z: %s\n",
					        optarg);
					exit(1);
				} else {
					_options.zoom = z / 100.0;
				}
				break;
		}
	}

	if (!_options.winw) {
		_options.winw = WIN_WIDTH;
		_options.winh = WIN_HEIGHT;
	} else if (!_options.winh) {
		_options.winh = _options.winw;
	}

	_options.filenames = (const char**) argv + optind;
	_options.filecnt = argc - optind;
}
