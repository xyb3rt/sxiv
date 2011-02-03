/* sxiv: util.c
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

#include <stdlib.h>
#include <stdio.h>

#include "options.h"
#include "util.h"

void cleanup();

void* s_malloc(size_t size) {
	void *ptr;
	
	if (!(ptr = malloc(size)))
		die("could not allocate memory");
	return ptr;
}

void* s_realloc(void *ptr, size_t size) {
	if (!(ptr = realloc(ptr, size)))
		die("could not allocate memory");
	return ptr;
}

void warn(const char* fmt, ...) {
	va_list args;

	if (!fmt || options->quiet)
		return;

	va_start(args, fmt);
	fprintf(stderr, "sxiv: warning: ");
	fprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
}

void die(const char* fmt, ...) {
	va_list args;

	if (!fmt)
		return;

	va_start(args, fmt);
	fprintf(stderr, "sxiv: error: ");
	fprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);

	cleanup();
	exit(1);
}
