/* sxiv: util.c
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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "options.h"
#include "util.h"

enum {
	DNAME_CNT = 512,
	FNAME_LEN = 1024
};

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

char* s_strdup(char *s) {
	char *d = NULL;

	if (s) {
		if (!(d = malloc(strlen(s) + 1)))
			die("could not allocate memory");
		strcpy(d, s);
	}
	return d;
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

	for (i = 0; i < LEN(units) && *size > 1024; i++)
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

int r_opendir(r_dir_t *rdir, const char *dirname) {
	if (!rdir || !dirname || !*dirname)
		return -1;

	if (!(rdir->dir = opendir(dirname))) {
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

	if (!rdir)
		return -1;
	
	if (rdir->stack) {
		while (rdir->stlen > 0)
			free(rdir->stack[--rdir->stlen]);
		free(rdir->stack);
		rdir->stack = NULL;
	}

	if (rdir->dir) {
		if (!(ret = closedir(rdir->dir)))
			rdir->dir = NULL;
	}

	if (rdir->d && rdir->name) {
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

	if (!rdir || !rdir->dir || !rdir->name)
		return NULL;

	while (1) {
		if (rdir->dir && (dentry = readdir(rdir->dir))) {
			if (!strcmp(dentry->d_name, ".") || !strcmp(dentry->d_name, ".."))
				continue;

			len = strlen(rdir->name) + strlen(dentry->d_name) + 2;
			filename = (char*) s_malloc(len);
			snprintf(filename, len, "%s%s%s", rdir->name,
			         rdir->name[strlen(rdir->name)-1] == '/' ? "" : "/",
			         dentry->d_name);

			if (!stat(filename, &fstats) && S_ISDIR(fstats.st_mode)) {
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
			if (rdir->d)
				free(rdir->name);
			rdir->name = rdir->stack[--rdir->stlen];
			rdir->d = 1;
			if (!(rdir->dir = opendir(rdir->name)))
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
