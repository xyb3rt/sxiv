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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <libgen.h>
#include <time.h>

#include "util.h"
#include "autoreload.h"

const struct timespec ten_ms = {0, 10000000};

void arl_cleanup(void)
{
	if (autoreload.fd != -1 && autoreload.wd != -1)
	{
		if(inotify_rm_watch(autoreload.fd, autoreload.wd))
			error(0, 0, "Failed to remove inotify watch.");
	}
}

void arl_handle(void)
{
	ssize_t len;
	char buf[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
	const struct inotify_event *event;
	char *ptr;
	char *fntmp, *fn;

	len = read(autoreload.fd, buf, sizeof buf);
	if (len == -1)
	{
		error(0, 0, "Failed to read inotify events.");
		return;
	}

	for (ptr = buf; ptr < buf + len;
			ptr += sizeof(struct inotify_event) + event->len)
	{

		event = (const struct inotify_event *) ptr;

		/* events from watching the file itself */
		if (event->mask & IN_CLOSE_WRITE)
		{
			load_image(fileidx);
			redraw();
		}

		if (event->mask & IN_DELETE_SELF)
			arl_setup_dir();

		/* events from watching the file's directory */
		if (event->mask & IN_CREATE)
		{
			fntmp = strdup(files[fileidx].path);
			fn = basename(fntmp);

			if (0 == strcmp(event->name, fn))
			{
				/* this is the file we're looking for */

				/* cleanup, this has not been one-shot */
				if (autoreload.watching_dir)
				{
					if(inotify_rm_watch(autoreload.fd, autoreload.wd))
						error(0, 0, "Failed to remove inotify watch.");
					autoreload.watching_dir = false;
				}

				/* when too fast, imlib2 can't load the image */
				nanosleep(&ten_ms, NULL);
				load_image(fileidx);
				redraw();
			}
			free(fntmp);
		}
	}
}

void arl_init(void)
{
	/* this needs to be done only once */
	autoreload.fd = inotify_init();
	autoreload.watching_dir = false;
	if (autoreload.fd == -1)
		error(0, 0, "Could not initialize inotify.");
}

void arl_setup(void)
{
	if (autoreload.fd == -1)
	{
		error(0, 0, "Uninitialized, could not add inotify watch.");
		return;
	}

	/* may have switched from a deleted to another image */
	if (autoreload.watching_dir)
	{
		if (inotify_rm_watch(autoreload.fd, autoreload.wd))
			error(0, 0, "Failed to remove inotify watch.");
		autoreload.watching_dir = false;
	}

	autoreload.wd = inotify_add_watch(autoreload.fd, files[fileidx].path,
		IN_ONESHOT | IN_CLOSE_WRITE | IN_DELETE_SELF);
	if (autoreload.wd == -1)
		error(0, 0, "Failed to add inotify watch on file '%s'.", files[fileidx].path);
}

void arl_setup_dir(void)
{
	char *dntmp, *dn;

	if (autoreload.fd == -1)
	{
		error(0, 0, "Uninitialized, could not add inotify watch on directory.");
		return;
	}

	/* get dirname */
	dntmp = (char*) strdup(files[fileidx].path);
	dn = (char*) dirname(dntmp);

	/* this is not one-shot as other stuff may be created too
	   note: we won't handle deletion of the directory itself,
	     this is a design decision								*/
	autoreload.wd = inotify_add_watch(autoreload.fd, dn,IN_CREATE);
	if (autoreload.wd == -1)
		error(0, 0, "Failed to add inotify watch on directory '%s'.", dn);
	else
		autoreload.watching_dir = true;

	free(dntmp);
}
