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

#define _POSIX_C_SOURCE 200112L
#define _IMAGE_CONFIG

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "options.h"
#include "util.h"
#include "config.h"

options_t _options;
const options_t *options = (const options_t*) &_options;

void print_usage(void)
{
	printf("usage: sxiv [-bcFfhioqrtvZ] [-G GAMMA] [-g GEOMETRY] [-n NUM] "
	       "[-N NAME] [-S DELAY] [-s MODE] [-z ZOOM] FILES...\n");
}

void print_version(void)
{
	printf("sxiv %s - Simple X Image Viewer\n", VERSION);
}

void parse_options(int argc, char **argv)
{
	int n, opt;
	char *end, *s;
	const char *scalemodes = "dfwh";

	_options.from_stdin = false;
	_options.to_stdout = false;
	_options.recursive = false;
	_options.startnum = 0;

	_options.scalemode = SCALE_DOWN;
	_options.zoom = 1.0;
	_options.gamma = 0;
	_options.slideshow = 0;

	_options.fixed_win = false;
	_options.fullscreen = false;
	_options.hide_bar = false;
	_options.geometry = NULL;
	_options.res_name = NULL;

	_options.quiet = false;
	_options.thumb_mode = false;
	_options.clean_cache = false;

	while ((opt = getopt(argc, argv, "bcFfG:g:hin:N:oqrS:s:tvZz:")) != -1) {
		switch (opt) {
			case '?':
				print_usage();
				exit(EXIT_FAILURE);
			case 'b':
				_options.hide_bar = true;
				break;
			case 'c':
				_options.clean_cache = true;
				break;
			case 'F':
				_options.fixed_win = true;
				break;
			case 'f':
				_options.fullscreen = true;
				break;
			case 'G':
				n = strtol(optarg, &end, 0);
				if (*end != '\0') {
					fprintf(stderr, "sxiv: invalid argument for option -G: %s\n", optarg);
					exit(EXIT_FAILURE);
				}
				_options.gamma = n;
				break;
			case 'g':
				_options.geometry = optarg;
				break;
			case 'h':
				print_usage();
				exit(EXIT_SUCCESS);
			case 'i':
				_options.from_stdin = true;
				break;
			case 'n':
				n = strtol(optarg, &end, 0);
				if (*end != '\0' || n <= 0) {
					fprintf(stderr, "sxiv: invalid argument for option -n: %s\n", optarg);
					exit(EXIT_FAILURE);
				}
				_options.startnum = n - 1;
				break;
			case 'N':
				_options.res_name = optarg;
				break;
			case 'o':
				_options.to_stdout = true;
				break;
			case 'q':
				_options.quiet = true;
				break;
			case 'r':
				_options.recursive = true;
				break;
			case 'S':
				n = strtol(optarg, &end, 0);
				if (*end != '\0' || n <= 0) {
					fprintf(stderr, "sxiv: invalid argument for option -S: %s\n", optarg);
					exit(EXIT_FAILURE);
				}
				_options.slideshow = n;
				break;
			case 's':
				s = strchr(scalemodes, optarg[0]);
				if (s == NULL || *s == '\0' || strlen(optarg) != 1) {
					fprintf(stderr, "sxiv: invalid argument for option -s: %s\n", optarg);
					exit(EXIT_FAILURE);
				}
				_options.scalemode = s - scalemodes;
				break;
			case 't':
				_options.thumb_mode = true;
				break;
			case 'v':
				print_version();
				exit(EXIT_SUCCESS);
			case 'Z':
				_options.scalemode = SCALE_ZOOM;
				_options.zoom = 1.0;
				break;
			case 'z':
				n = strtol(optarg, &end, 0);
				if (*end != '\0' || n <= 0) {
					fprintf(stderr, "sxiv: invalid argument for option -z: %s\n", optarg);
					exit(EXIT_FAILURE);
				}
				_options.scalemode = SCALE_ZOOM;
				_options.zoom = (float) n / 100.0;
				break;
		}
	}

	_options.filenames = argv + optind;
	_options.filecnt = argc - optind;

	if (_options.filecnt == 1 && STREQ(_options.filenames[0], "-")) {
		_options.filenames++;
		_options.filecnt--;
		_options.from_stdin = true;
	}
}
