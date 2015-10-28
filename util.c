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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "options.h"
#include "util.h"

void cleanup(void);

void* emalloc(size_t size)
{
	void *ptr;
	
	ptr = malloc(size);
	if (ptr == NULL)
		die("could not allocate memory");
	return ptr;
}

void* erealloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);
	if (ptr == NULL)
		die("could not allocate memory");
	return ptr;
}

char* estrdup(const char *s)
{
	char *d;
	size_t n = strlen(s) + 1;

	d = malloc(n);
	if (d == NULL)
		die("could not allocate memory");
	memcpy(d, s, n);
	return d;
}

void warn(const char* fmt, ...)
{
	va_list args;

	if (fmt == NULL || options->quiet)
		return;

	va_start(args, fmt);
	fprintf(stderr, "sxiv: warning: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
}

void die(const char* fmt, ...)
{
	va_list args;

	if (fmt == NULL)
		return;

	va_start(args, fmt);
	fprintf(stderr, "sxiv: error: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);

	cleanup();
	exit(EXIT_FAILURE);
}

void size_readable(float *size, const char **unit)
{
	const char *units[] = { "", "K", "M", "G" };
	int i;

	for (i = 0; i < ARRLEN(units) && *size > 1024.0; i++)
		*size /= 1024.0;
	*unit = units[MIN(i, ARRLEN(units) - 1)];
}

int r_opendir(r_dir_t *rdir, const char *dirname)
{
	if (*dirname == '\0')
		return -1;

	if ((rdir->dir = opendir(dirname)) == NULL) {
		rdir->name = NULL;
		rdir->stack = NULL;
		return -1;
	}

	rdir->stcap = 512;
	rdir->stack = (char**) emalloc(rdir->stcap * sizeof(char*));
	rdir->stlen = 0;

	rdir->name = (char*) dirname;
	rdir->d = 0;

	return 0;
}

int r_closedir(r_dir_t *rdir)
{
	int ret = 0;

	if (rdir->stack != NULL) {
		while (rdir->stlen > 0)
			free(rdir->stack[--rdir->stlen]);
		free(rdir->stack);
		rdir->stack = NULL;
	}

	if (rdir->dir != NULL) {
		if ((ret = closedir(rdir->dir)) == 0)
			rdir->dir = NULL;
	}

	if (rdir->d != 0) {
		free(rdir->name);
		rdir->name = NULL;
	}

	return ret;
}

char* r_readdir(r_dir_t *rdir)
{
	size_t len;
	char *filename;
	struct dirent *dentry;
	struct stat fstats;

	while (true) {
		if (rdir->dir != NULL && (dentry = readdir(rdir->dir)) != NULL) {
			if (dentry->d_name[0] == '.')
				continue;

			len = strlen(rdir->name) + strlen(dentry->d_name) + 2;
			filename = (char*) emalloc(len);
			snprintf(filename, len, "%s%s%s", rdir->name,
			         rdir->name[strlen(rdir->name)-1] == '/' ? "" : "/",
			         dentry->d_name);

			if (stat(filename, &fstats) < 0)
				continue;
			if (S_ISDIR(fstats.st_mode)) {
				/* put subdirectory on the stack */
				if (rdir->stlen == rdir->stcap) {
					rdir->stcap *= 2;
					rdir->stack = (char**) erealloc(rdir->stack,
					                                rdir->stcap * sizeof(char*));
				}
				rdir->stack[rdir->stlen++] = filename;
				continue;
			}
			return filename;
		}
		
		if (rdir->stlen > 0) {
			/* open next subdirectory */
			closedir(rdir->dir);
			if (rdir->d != 0)
				free(rdir->name);
			rdir->name = rdir->stack[--rdir->stlen];
			rdir->d = 1;
			if ((rdir->dir = opendir(rdir->name)) == NULL)
				warn("could not open directory: %s", rdir->name);
			continue;
		}
		/* no more entries */
		break;
	}
	return NULL;
}

int r_mkdir(const char *path)
{
	char *dir, *d;
	struct stat stats;
	int err = 0;

	if (*path == '\0')
		return -1;

	if (stat(path, &stats) == 0)
		return S_ISDIR(stats.st_mode) ? 0 : -1;

	d = dir = (char*) emalloc(strlen(path) + 1);
	strcpy(dir, path);

	while (d != NULL && err == 0) {
		d = strchr(d + 1, '/');
		if (d != NULL)
			*d = '\0';
		if (access(dir, F_OK) < 0 && errno == ENOENT) {
			if (mkdir(dir, 0755) < 0) {
				warn("could not create directory: %s", dir);
				err = -1;
			}
		} else if (stat(dir, &stats) < 0 || !S_ISDIR(stats.st_mode)) {
			err = -1;
		}
		if (d != NULL)
			*d = '/';
	}
	free(dir);

	return err;
}
