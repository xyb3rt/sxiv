/* sxiv: util.c
 * Copyright (c) 2012 Bert Muennich <be.muennich at googlemail.com>
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

#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "options.h"
#include "util.h"

enum {
	BUF_SIZE  = 1024,
	DNAME_CNT = 512,
	FNAME_LEN = 1024
};

void cleanup(void);

void* s_malloc(size_t size) {
	void *ptr;
	
	ptr = malloc(size);
	if (ptr == NULL)
		die("could not allocate memory");
	return ptr;
}

void* s_realloc(void *ptr, size_t size) {
	ptr = realloc(ptr, size);
	if (ptr == NULL)
		die("could not allocate memory");
	return ptr;
}

char* s_strdup(char *s) {
	char *d = NULL;

	if (s != NULL) {
		d = malloc(strlen(s) + 1);
		if (d == NULL)
			die("could not allocate memory");
		strcpy(d, s);
	}
	return d;
}

void warn(const char* fmt, ...) {
	va_list args;

	if (fmt == NULL || options->quiet)
		return;

	va_start(args, fmt);
	fprintf(stderr, "sxiv: warning: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
}

void die(const char* fmt, ...) {
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

ssize_t get_line(char **buf, size_t *n, FILE *stream) {
	size_t len;
	char *s;

	if (stream == NULL || feof(stream) || ferror(stream))
		return -1;

	if (*buf == NULL || *n == 0) {
		*n = BUF_SIZE;
		*buf = (char*) s_malloc(*n);
	}
	s = *buf;

	while (true) {
		if (fgets(s, *n - (s - *buf), stream) == NULL)
			return -1;
		len = strlen(s);
		if (feof(stream))
			break;
		if (len > 0 && s[len-1] == '\n')
			break;
		if (len + 1 == *n - (s - *buf)) {
			*buf = (char*) s_realloc(*buf, 2 * *n);
			s = *buf + *n - 1;
			*n *= 2;
		} else {
			s += len;
		}
	}
	return s - *buf + len;
}

void size_readable(float *size, const char **unit) {
	const char *units[] = { "", "K", "M", "G" };
	int i;

	for (i = 0; i < ARRLEN(units) && *size > 1024.0; i++)
		*size /= 1024.0;
	*unit = units[MIN(i, ARRLEN(units) - 1)];
}

char* absolute_path(const char *filename) {
	size_t len;
	const char *basename;
	char *dir, *dirname = NULL, *path = NULL, *s;
	char *cwd = NULL, *twd = NULL;

	if (filename == NULL || *filename == '\0' || *filename == '/')
		return NULL;

	len = FNAME_LEN;
	cwd = (char*) s_malloc(len);
	while ((s = getcwd(cwd, len)) == NULL && errno == ERANGE) {
		len *= 2;
		cwd = (char*) s_realloc(cwd, len);
	}
	if (s == NULL)
		goto error;

	s = strrchr(filename, '/');
	if (s != NULL) {
		len = s - filename;
		dirname = (char*) s_malloc(len + 1);
		strncpy(dirname, filename, len);
		dirname[len] = '\0';
		basename = s + 1;

		if (chdir(cwd) < 0)
			/* we're not able to come back afterwards */
			goto error;
		if (chdir(dirname) < 0)
			goto error;

		len = FNAME_LEN;
		twd = (char*) s_malloc(len);
		while ((s = getcwd(twd, len)) == NULL && errno == ERANGE) {
			len *= 2;
			twd = (char*) s_realloc(twd, len);
		}
		if (chdir(cwd) < 0)
			die("could not revert to prior working directory");
		if (s == NULL)
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
	if (path != NULL) {
		free(path);
		path = NULL;
	}

end:
	if (dirname != NULL)
		free(dirname);
	if (cwd != NULL)
		free(cwd);
	if (twd != NULL)
		free(twd);

	return path;
}

int r_opendir(r_dir_t *rdir, const char *dirname) {
	if (rdir == NULL || dirname == NULL || *dirname == '\0')
		return -1;

	if ((rdir->dir = opendir(dirname)) == NULL) {
		rdir->name = NULL;
		rdir->stack = NULL;
		return -1;
	}

	rdir->stcap = DNAME_CNT;
	rdir->stack = (char**) s_malloc(rdir->stcap * sizeof(char*));
	rdir->stlen = 0;

	rdir->name = (char*) dirname;
	rdir->d = 0;

	return 0;
}

int r_closedir(r_dir_t *rdir) {
	int ret = 0;

	if (rdir == NULL)
		return -1;
	
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

	if (rdir->d != 0 && rdir->name != NULL) {
		free(rdir->name);
		rdir->name = NULL;
	}

	return ret;
}

char* r_readdir(r_dir_t *rdir) {
	size_t len;
	char *filename;
	struct dirent *dentry;
	struct stat fstats;

	if (rdir == NULL || rdir->dir == NULL || rdir->name == NULL)
		return NULL;

	while (true) {
		if (rdir->dir != NULL && (dentry = readdir(rdir->dir)) != NULL) {
			if (STREQ(dentry->d_name, ".") || STREQ(dentry->d_name, ".."))
				continue;

			len = strlen(rdir->name) + strlen(dentry->d_name) + 2;
			filename = (char*) s_malloc(len);
			snprintf(filename, len, "%s%s%s", rdir->name,
			         rdir->name[strlen(rdir->name)-1] == '/' ? "" : "/",
			         dentry->d_name);

			if (stat(filename, &fstats) < 0)
				continue;
			if (S_ISDIR(fstats.st_mode)) {
				/* put subdirectory on the stack */
				if (rdir->stlen == rdir->stcap) {
					rdir->stcap *= 2;
					rdir->stack = (char**) s_realloc(rdir->stack,
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

int r_mkdir(const char *path) {
	char *dir, *d;
	struct stat stats;
	int err = 0;

	if (path == NULL || *path == '\0')
		return -1;

	if (stat(path, &stats) == 0) {
		if (S_ISDIR(stats.st_mode)) {
			return 0;
		} else {
			warn("not a directory: %s", path);
			return -1;
		}
	}

	d = dir = (char*) s_malloc(strlen(path) + 1);
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
			warn("not a directory: %s", dir);
			err = -1;
		}
		if (d != NULL)
			*d = '/';
	}
	free(dir);

	return err;
}
