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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "options.h"
#include "util.h"

#define FNAME_LEN 1024

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

char* absolute_path(const char *filename) {
	size_t len;
	char *path = NULL;
	const char *basename;
	char *dirname = NULL;
	char *cwd = NULL;
	char *twd = NULL;
	char *dir;
	char *s;

	if (!filename || *filename == '\0' || *filename == '/')
		return NULL;

	len = FNAME_LEN;
	cwd = (char*) s_malloc(len);
	while (!(s = getcwd(cwd, len)) && errno == ERANGE) {
		len *= 2;
		cwd = (char*) s_realloc(cwd, len);
	}
	if (!s)
		goto error;

	s = strrchr(filename, '/');
	if (s) {
		len = s - filename;
		dirname = (char*) s_malloc(len + 1);
		strncpy(dirname, filename, len);
		dirname[len] = '\0';
		basename = s + 1;

		if (chdir(cwd))
			/* we're not able to come back afterwards */
			goto error;
		if (chdir(dirname))
			goto error;

		len = FNAME_LEN;
		twd = (char*) s_malloc(len);
		while (!(s = getcwd(twd, len)) && errno == ERANGE) {
			len *= 2;
			twd = (char*) s_realloc(twd, len);
		}
		if (chdir(cwd))
			die("could not revert to prior working directory");
		if (!s)
			goto error;
		dir = twd;
	} else {
		/* only a single filename given */
		basename = filename;
		dir = cwd;
	}

	len = strlen(dir) + strlen(basename) + 2;
	path = (char*) s_malloc(len);
	snprintf(path, len, "%s/%s", dir, basename);

goto end;

error:
	if (path) {
		free(path);
		path = NULL;
	}

end:
	if (dirname)
		free(dirname);
	if (cwd)
		free(cwd);
	if (twd)
		free(twd);

	return path;
}

int create_dir_rec(const char *path) {
	char *dir, *d;
	struct stat stats;
	int err = 0;

	if (!path || !*path)
		return -1;

	if (!stat(path, &stats)) {
		if (S_ISDIR(stats.st_mode)) {
			return 0;
		} else {
			warn("not a directory: %s", path);
			return -1;
		}
	}

	d = dir = (char*) s_malloc(strlen(path) + 1);
	strcpy(dir, path);

	while (d != NULL && !err) {
		d = strchr(d + 1, '/');
		if (d != NULL)
			*d = '\0';
		if (access(dir, F_OK) && errno == ENOENT) {
			if (mkdir(dir, 0755)) {
				warn("could not create directory: %s", dir);
				err = -1;
			}
		} else if (stat(dir, &stats) || !S_ISDIR(stats.st_mode)) {
			warn("not a directory: %s", dir);
			err = -1;
		}
		if (d != NULL)
			*d = '/';
	}
	free(dir);

	return err;
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

	if (ferror(stream)) {
		s = NULL;
	} else {
		s = (char*) s_malloc((strlen(buf) + 1) * sizeof(char));
		strcpy(s, buf);
	}

	free(buf);

	return s;
}
