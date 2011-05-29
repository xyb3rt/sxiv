/* sxiv: util.h
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

#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdarg.h>
#include <dirent.h>

#define ABS(a)   ((a) < 0 ? (-(a)) : (a))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define LEN(a)   (sizeof(a) / sizeof(a[0]))

#define TV_TO_DOUBLE(x) ((double) ((x).tv_sec) + 0.000001 * \
                         (double) ((x).tv_usec))

#define TIMESPEC_TO_TIMEVAL(tv, ts) {      \
		(tv)->tv_sec = (ts)->tv_sec;           \
		(tv)->tv_usec = (ts)->tv_nsec / 1000;  \
}

typedef struct {
	DIR *dir;
	char *name;
	int d;

	char **stack;
	int stcap;
	int stlen;
} r_dir_t;

void* s_malloc(size_t);
void* s_realloc(void*, size_t);

void warn(const char*, ...);
void die(const char*, ...);

void size_readable(float*, const char**);

char* absolute_path(const char*);

int r_opendir(r_dir_t*, const char*);
int r_closedir(r_dir_t*);
char* r_readdir(r_dir_t*);
int r_mkdir(const char *);

#endif /* UTIL_H */
