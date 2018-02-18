/* Copyright 2017 Max Voit, Bert Muennich
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

#include "sxiv.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>

void arl_init(arl_t *arl)
{
	arl->fd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
	arl->wd_dir = arl->wd_file = -1;
	if (arl->fd == -1)
		error(0, 0, "Could not initialize inotify, no automatic image reloading");
}

CLEANUP void arl_cleanup(arl_t *arl)
{
	if (arl->fd != -1)
		close(arl->fd);
	free(arl->filename);
}

static void rm_watch(int fd, int *wd)
{
	if (*wd != -1) {
		inotify_rm_watch(fd, *wd);
		*wd = -1;
	}
}

static void add_watch(int fd, int *wd, const char *path, uint32_t mask)
{
	*wd = inotify_add_watch(fd, path, mask);
	if (*wd == -1)
		error(0, errno, "inotify: %s", path);
}

void arl_setup(arl_t *arl, const char *filepath)
{
	char *base = strrchr(filepath, '/');

	if (arl->fd == -1)
		return;

	rm_watch(arl->fd, &arl->wd_dir);
	rm_watch(arl->fd, &arl->wd_file);

	add_watch(arl->fd, &arl->wd_file, filepath, IN_CLOSE_WRITE | IN_DELETE_SELF);

	free(arl->filename);
	arl->filename = estrdup(filepath);

	if (base != NULL) {
		arl->filename[++base - filepath] = '\0';
		add_watch(arl->fd, &arl->wd_dir, arl->filename, IN_CREATE | IN_MOVED_TO);
		strcpy(arl->filename, base);
	}
}

union {
	char d[4096]; /* aligned buffer */
	struct inotify_event e;
} buf;

bool arl_handle(arl_t *arl)
{
	bool reload = false;
	char *ptr;
	const struct inotify_event *e;

	for (;;) {
		ssize_t len = read(arl->fd, buf.d, sizeof(buf.d));

		if (len == -1) {
			if (errno == EINTR)
				continue;
			break;
		}
		for (ptr = buf.d; ptr < buf.d + len; ptr += sizeof(*e) + e->len) {
			e = (const struct inotify_event*) ptr;
			if (e->wd == arl->wd_file && (e->mask & IN_CLOSE_WRITE)) {
				reload = true;
			} else if (e->wd == arl->wd_file && (e->mask & IN_DELETE_SELF)) {
				rm_watch(arl->fd, &arl->wd_file);
			} else if (e->wd == arl->wd_dir && (e->mask & (IN_CREATE | IN_MOVED_TO))) {
				if (STREQ(e->name, arl->filename))
					reload = true;
			}
		}
	}
	return reload;
}

