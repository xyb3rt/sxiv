/* Copyright 2017 Max Voit
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <libgen.h>

#include "util.h"
#include "autoreload.h"

CLEANUP void arl_cleanup(arl_t *arl)
{
	if (arl->fd != -1)
		close(arl->fd);
}

static void arl_setup_dir(arl_t *arl, const char *filepath)
{
	char *dntmp, *dn;

	if (arl->fd == -1)
		return;

	/* get dirname */
	dntmp = (char*) strdup(filepath);
	dn = (char*) dirname(dntmp);

	/* this is not one-shot as other stuff may be created too
	 * note: we won't handle deletion of the directory itself,
	 * this is a design decision
	 */
	arl->wd = inotify_add_watch(arl->fd, dn, IN_CREATE);
	if (arl->wd == -1)
		error(0, 0, "%s: Error watching directory", dn);
	else
		arl->watching_dir = true;

	free(dntmp);
}

union {
	char d[4096]; /* aligned buffer */
	struct inotify_event e;
} buf;

bool arl_handle(arl_t *arl, const char *filepath)
{
	bool reload = false;
	char *ptr;
	const struct inotify_event *event;

	for (;;) {
		ssize_t len = read(arl->fd, buf.d, sizeof(buf.d));

		if (len == -1) {
			if (errno == EINTR)
				continue;
			break;
		}
		for (ptr = buf.d; ptr < buf.d + len; ptr += sizeof(*event) + event->len) {
			event = (const struct inotify_event*) ptr;

			/* events from watching the file itself */
			if (event->mask & IN_CLOSE_WRITE) {
				reload = true;
			}
			if (event->mask & IN_DELETE_SELF)
				arl_setup_dir(arl, filepath);

			/* events from watching the file's directory */
			if (event->mask & IN_CREATE) {
				char *fntmp = strdup(filepath);
				char *fn = basename(fntmp);

				if (STREQ(event->name, fn)) {
					/* this is the file we're looking for */

					/* cleanup, this has not been one-shot */
					if (arl->watching_dir) {
						inotify_rm_watch(arl->fd, arl->wd);
						arl->watching_dir = false;
					}
					reload = true;
				}
				free(fntmp);
			}
		}
	}
	return reload;
}

void arl_init(arl_t *arl)
{
	/* this needs to be done only once */
	arl->fd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
	arl->watching_dir = false;
	if (arl->fd == -1)
		error(0, 0, "Could not initialize inotify, no automatic image reloading");
}

void arl_setup(arl_t *arl, const char *filepath)
{
	if (arl->fd == -1)
		return;

	/* may have switched from a deleted to another image */
	if (arl->watching_dir) {
		inotify_rm_watch(arl->fd, arl->wd);
		arl->watching_dir = false;
	}

	arl->wd = inotify_add_watch(arl->fd, filepath,
		                          IN_ONESHOT | IN_CLOSE_WRITE | IN_DELETE_SELF);
	if (arl->wd == -1)
		error(0, 0, "%s: Error watching file", filepath);
}

