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
#include <string.h>

#include "options.h"
#include "util.h"

#define FNAME_LEN 10

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
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
}

void die(const char* fmt, ...) {
	va_list args;

	if (!fmt)
		return;

	va_start(args, fmt);
	fprintf(stderr, "sxiv: error: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);

	cleanup();
	exit(1);
}

void size_readable(float *size, const char **unit) {
	const char *units[] = { "", "K", "M", "G" };
	int i;

	for (i = 0; i < LEN(units) && *size > 1024; ++i)
		*size /= 1024;
	*unit = units[MIN(i, LEN(units) - 1)];
}

char* readline(FILE *stream) {
	size_t len;
	char *buf, *s, *end;

	if (!stream || feof(stream) || ferror(stream))
		return NULL;

	len = FNAME_LEN;
	s = buf = (char*) s_malloc(len * sizeof(char));

	do {
		*s = '\0';
		fgets(s, len - (s - buf), stream);
		if ((end = strchr(s, '\n'))) {
			*end = '\0';
		} else if (strlen(s) + 1 == len - (s - buf)) {
			buf = (char*) s_realloc(buf, 2 * len * sizeof(char));
			s = buf + len - 1;
			len *= 2;
		} else {
			s += strlen(s);
		}
	} while (!end && !feof(stream) && !ferror(stream));

	if (!ferror(stream) && *buf) {
		s = (char*) s_malloc((strlen(buf) + 1) * sizeof(char));
		strcpy(s, buf);
	} else {
		s = NULL;
	}

	free(buf);

	return s;
}
