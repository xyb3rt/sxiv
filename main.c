/* sxiv: main.c
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
#include <unistd.h>
#include <sys/stat.h>

#include "events.h"
#include "image.h"
#include "options.h"
#include "thumbs.h"
#include "util.h"
#include "window.h"

enum {
	TITLE_LEN = 256,
	FNAME_CNT = 1024
};

appmode_t mode;
img_t img;
tns_t tns;
win_t win;

char **filenames;
int filecnt, fileidx;
size_t filesize;

char win_title[TITLE_LEN];

void cleanup() {
	static int in = 0;

	if (!in++) {
		img_close(&img, 0);
		tns_free(&tns);
		win_close(&win);
	}
}

int check_add_file(char *filename) {
	if (!filename)
		return 0;

	if (access(filename, R_OK)) {
		warn("could not open file: %s", filename);
		return 0;
	} else {
		if (fileidx == filecnt) {
			filecnt *= 2;
			filenames = (char**) s_realloc(filenames, filecnt * sizeof(char*));
		}
		filenames[fileidx++] = filename;
		return 1;
	}
}

void remove_file(int n, unsigned char silent) {
	if (n < 0 || n >= filecnt)
		return;

	if (filecnt == 1) {
		if (!silent)
			fprintf(stderr, "sxiv: no more files to display, aborting\n");
		cleanup();
		exit(!silent);
	}

	if (n + 1 < filecnt)
		memmove(filenames + n, filenames + n + 1, (filecnt - n - 1) *
		        sizeof(char*));
	if (n + 1 < tns.cnt) {
		memmove(tns.thumbs + n, tns.thumbs + n + 1, (tns.cnt - n - 1) *
		        sizeof(thumb_t));
		memset(tns.thumbs + tns.cnt - 1, 0, sizeof(thumb_t));
	}

	filecnt--;
	if (n < tns.cnt)
		tns.cnt--;
}

void load_image(int new) {
	struct stat fstats;

	if (new < 0 || new >= filecnt)
		return;

	/* cursor is reset in redraw() */
	win_set_cursor(&win, CURSOR_WATCH);
	img_close(&img, 0);
		
	while (!img_load(&img, filenames[new])) {
		remove_file(new, 0);
		if (new >= filecnt)
			new = filecnt - 1;
	}

	fileidx = new;
	if (!stat(filenames[new], &fstats))
		filesize = fstats.st_size;
	else
		filesize = 0;
}

void update_title() {
	int n;
	float size;
	const char *unit;

	if (mode == MODE_THUMBS) {
		n = snprintf(win_title, TITLE_LEN, "sxiv: [%d/%d] %s",
		             tns.cnt ? tns.sel + 1 : 0, tns.cnt,
		             tns.cnt ? filenames[tns.sel] : "");
	} else {
		size = filesize;
		size_readable(&size, &unit);
		n = snprintf(win_title, TITLE_LEN,
		             "sxiv: [%d/%d] <%d%%> <%dx%d> (%.2f%s) %s",
		             fileidx + 1, filecnt, (int) (img.zoom * 100.0), img.w, img.h,
		             size, unit, filenames[fileidx]);
	}

	if (n >= TITLE_LEN) {
		for (n = 0; n < 3; n++)
			win_title[TITLE_LEN - n - 2] = '.';
	}

	win_set_title(&win, win_title);
}

int fncmp(const void *a, const void *b) {
	return strcoll(*((char* const*) a), *((char* const*) b));
}

int main(int argc, char **argv) {
	int i, len, start;
	size_t n;
	char *filename = NULL;
	struct stat fstats;
	r_dir_t dir;

	parse_options(argc, argv);

	if (options->clean_cache) {
		tns_init(&tns, 0);
		tns_clean_cache(&tns);
		exit(0);
	}

	if (!options->filecnt) {
		print_usage();
		exit(1);
	}

	if (options->recursive || options->from_stdin)
		filecnt = FNAME_CNT;
	else
		filecnt = options->filecnt;

	filenames = (char**) s_malloc(filecnt * sizeof(char*));
	fileidx = 0;

	/* build file list: */
	if (options->from_stdin) {
		while ((len = getline(&filename, &n, stdin)) > 0) {
			if (filename[len-1] == '\n')
				filename[len-1] = '\0';
			if (!*filename || !check_add_file(filename))
				free(filename);
			filename = NULL;
		}
	} else {
		for (i = 0; i < options->filecnt; i++) {
			filename = options->filenames[i];

			if (stat(filename, &fstats) || !S_ISDIR(fstats.st_mode)) {
				check_add_file(filename);
			} else {
				if (!options->recursive) {
					warn("ignoring directory: %s", filename);
					continue;
				}
				if (r_opendir(&dir, filename)) {
					warn("could not open directory: %s", filename);
					continue;
				}
				start = fileidx;
				while ((filename = r_readdir(&dir))) {
					if (!check_add_file(filename))
						free((void*) filename);
				}
				r_closedir(&dir);
				if (fileidx - start > 1)
					qsort(filenames + start, fileidx - start, sizeof(char*), fncmp);
			}
		}
	}

	if (!fileidx) {
		fprintf(stderr, "sxiv: no valid image file given, aborting\n");
		exit(1);
	}

	filecnt = fileidx;
	fileidx = options->startnum < filecnt ? options->startnum : 0;

	win_init(&win);
	img_init(&img, &win);

	if (options->thumbnails) {
		mode = MODE_THUMBS;
		tns_init(&tns, filecnt);
		while (!tns_load(&tns, 0, filenames[0], 0))
			remove_file(0, 0);
		tns.cnt = 1;
	} else {
		mode = MODE_NORMAL;
		tns.thumbs = NULL;
		load_image(fileidx);
	}

	win_open(&win);
	
	run();
	cleanup();

	return 0;
}
