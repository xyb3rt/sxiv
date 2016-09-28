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

#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/types.h>

#include "types.h"

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#define ARRLEN(a) (sizeof(a) / sizeof((a)[0]))

#define STREQ(s1,s2) (strcmp((s1), (s2)) == 0)

#define TV_DIFF(t1,t2) (((t1)->tv_sec  - (t2)->tv_sec ) * 1000 + \
                        ((t1)->tv_usec - (t2)->tv_usec) / 1000)

#define TV_SET_MSEC(tv,t) {             \
  (tv)->tv_sec  = (t) / 1000;           \
  (tv)->tv_usec = (t) % 1000 * 1000;    \
}

#define TV_ADD_MSEC(tv,t) {             \
  (tv)->tv_sec  += (t) / 1000;          \
  (tv)->tv_usec += (t) % 1000 * 1000;   \
}

typedef struct {
	DIR *dir;
	char *name;
	int d;
	bool recursive;

	char **stack;
	int stcap;
	int stlen;
} r_dir_t;

extern const char *progname;

void* emalloc(size_t);
void* erealloc(void*, size_t);
char* estrdup(const char*);

void error(int, int, const char*, ...);

void size_readable(float*, const char**);

int r_opendir(r_dir_t*, const char*, bool);
int r_closedir(r_dir_t*);
char* r_readdir(r_dir_t*);
int r_mkdir(char*);

#endif /* UTIL_H */
