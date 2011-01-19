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
	printf("usage: sxiv [-hv] FILES...\n");
}

void print_version() {
	printf("sxiv - simple x image viewer\n");
	printf("Version %s, written by Bert Muennich\n", VERSION);
}

void parse_options(int argc, char **argv) {
	int opt;

	_options.filenames = (const char**) argv + 1;
	_options.filecnt = argc - 1;

	while ((opt = getopt(argc, argv, "hv")) != -1) {
		switch (opt) {
			case '?':
				print_usage();
				exit(1);
			case 'h':
				print_usage();
				exit(0);
			case 'v':
				print_version();
				exit(0);
		}
	}
}
