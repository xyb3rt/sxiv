/* sxiv: util.h
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

#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdarg.h>
#include <dirent.h>

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#define ARRLEN(a) (sizeof(a) / sizeof((a)[0]))

#define STREQ(a,b) (!strcmp((a), (b)))

#define TIMEDIFF(t1,t2) (((t1)->tv_sec - (t2)->tv_sec) * 1000 +  \
                         ((t1)->tv_usec - (t2)->tv_usec) / 1000)

#define MSEC_TO_TIMEVAL(t,tv) {         \
  (tv)->tv_sec = (t) / 1000;            \
  (tv)->tv_usec = (t) % 1000 * 1000;    \
}

#define MSEC_ADD_TO_TIMEVAL(t,tv) {     \
  (tv)->tv_sec += (t) / 1000;           \
  (tv)->tv_usec += (t) % 1000 * 1000;   \
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
char* s_strdup(char*);

void warn(const char*, ...);
void die(const char*, ...);

ssize_t get_line(char**, size_t*, FILE*);

void size_readable(float*, const char**);
void time_readable(float*, const char**);

char* absolute_path(const char*);

int r_opendir(r_dir_t*, const char*);
int r_closedir(r_dir_t*);
char* r_readdir(r_dir_t*);
int r_mkdir(const char *);

#endif /* UTIL_H */
