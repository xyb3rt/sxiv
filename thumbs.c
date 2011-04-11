/* sxiv: thumbs.c
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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "thumbs.h"
#include "util.h"

const int thumb_dim = THUMB_SIZE + 10;
char *cache_dir = NULL;

int tns_cache_enabled() {
	struct stat stats;

	return cache_dir && !stat(cache_dir, &stats) && S_ISDIR(stats.st_mode) &&
	       !access(cache_dir, W_OK);
}

char* tns_cache_filename(const char *filename) {
	size_t len;
	char *cfile = NULL;
	const char *abspath;

	if (!cache_dir || !filename)
		return NULL;
	
	if (*filename != '/') {
		if (!(abspath = absolute_path(filename)))
			return NULL;
	} else {
		abspath = filename;
	}

	if (strncmp(abspath, cache_dir, strlen(cache_dir))) {
		len = strlen(cache_dir) + strlen(abspath) + 6;
		cfile = (char*) s_malloc(len);
		snprintf(cfile, len, "%s/%s.png", cache_dir, abspath + 1);
	}
	
	if (abspath != filename)
		free((void*) abspath);

	return cfile;
}

Imlib_Image* tns_cache_load(const char *filename) {
	char *cfile;
	struct stat cstats, fstats;
	Imlib_Image *im = NULL;

	if (!filename)
		return NULL;

	if (stat(filename, &fstats))
		return NULL;

	if ((cfile = tns_cache_filename(filename))) {
		if (!stat(cfile, &cstats) &&
		    cstats.st_mtim.tv_sec == fstats.st_mtim.tv_sec &&
				cstats.st_mtim.tv_nsec == fstats.st_mtim.tv_nsec)
		{
			im = imlib_load_image(cfile);
		}
		free(cfile);
	}

	return im;
}

void tns_cache_write(thumb_t *t, Bool force) {
	char *cfile, *dirend;
	struct stat cstats, fstats;
	struct timeval times[2];
	Imlib_Load_Error err = 0;

	if (!t || !t->im || !t->filename)
		return;

	if (stat(t->filename, &fstats))
		return;

	if ((cfile = tns_cache_filename(t->filename))) {
		if (force || stat(cfile, &cstats) ||
		    cstats.st_mtim.tv_sec != fstats.st_mtim.tv_sec ||
		    cstats.st_mtim.tv_nsec != fstats.st_mtim.tv_nsec)
		{
			if ((dirend = strrchr(cfile, '/'))) {
				*dirend = '\0';
				err = r_mkdir(cfile);
				*dirend = '/';
			}

			if (!err) {
				imlib_context_set_image(t->im);
				imlib_image_set_format("png");
				imlib_save_image_with_error_return(cfile, &err);
			}

			if (err) {
				warn("could not cache thumbnail: %s", t->filename);
			} else {
				TIMESPEC_TO_TIMEVAL(&times[0], &fstats.st_atim);
				TIMESPEC_TO_TIMEVAL(&times[1], &fstats.st_mtim);
				utimes(cfile, times);
			}
		}
		free(cfile);
	}
}

void tns_clean_cache(tns_t *tns) {
	int dirlen, delete;
	char *cfile, *filename, *tpos;
	r_dir_t dir;

	if (!cache_dir)
		return;
	
	if (r_opendir(&dir, cache_dir)) {
		warn("could not open thumbnail cache directory: %s", cache_dir);
		return;
	}

	dirlen = strlen(cache_dir);

	while ((cfile = r_readdir(&dir))) {
		filename = cfile + dirlen;
		delete = 0;

		if ((tpos = strrchr(filename, '.'))) {
			*tpos = '\0';
			delete = access(filename, F_OK);
			*tpos = '.';
		}

		if (delete && unlink(cfile))
			warn("could not delete cache file: %s", cfile);

		free(cfile);
	}

	r_closedir(&dir);
}


void tns_init(tns_t *tns, int cnt) {
	int len;
	char *homedir;

	if (!tns)
		return;

	if (cnt) {
		tns->thumbs = (thumb_t*) s_malloc(cnt * sizeof(thumb_t));
		memset(tns->thumbs, 0, cnt * sizeof(thumb_t));
	} else {
		tns->thumbs = NULL;
	}

	tns->cnt = tns->first = tns->sel = 0;
	tns->cap = cnt;
	tns->dirty = 0;

	if ((homedir = getenv("HOME"))) {
		if (cache_dir)
			free(cache_dir);
		len = strlen(homedir) + 10;
		cache_dir = (char*) s_malloc(len * sizeof(char));
		snprintf(cache_dir, len, "%s/.sxiv", homedir);
	} else {
		warn("could not locate thumbnail cache directory");
	}
}

void tns_free(tns_t *tns) {
	int i;

	if (!tns)
		return;

	if (tns->thumbs) {
		for (i = 0; i < tns->cnt; ++i) {
			if (tns->thumbs[i].im) {
				imlib_context_set_image(tns->thumbs[i].im);
				imlib_free_image();
			}
		}
		free(tns->thumbs);
		tns->thumbs = NULL;
	}

	if (cache_dir) {
		free(cache_dir);
		cache_dir = NULL;
	}
}

int tns_load(tns_t *tns, int n, const char *filename, unsigned char silent) {
	int w, h;
	int use_cache, cached = 0;
	float z, zw, zh;
	thumb_t *t;
	Imlib_Image *im;

	if (!tns || !tns->thumbs || !filename)
		return 0;

	if (n < 0 || n >= tns->cap)
		return 0;

	t = &tns->thumbs[n];
	t->filename = filename;

	if (t->im) {
		imlib_context_set_image(t->im);
		imlib_free_image();
	}

	if ((use_cache = tns_cache_enabled())) {
		if ((im = tns_cache_load(filename)))
			cached = 1;
	}

	if (!cached &&
	    (access(filename, R_OK) || !(im = imlib_load_image(filename))))
	{
		if (!silent)
			warn("could not open image: %s", filename);
		return 0;
	}

	imlib_context_set_image(im);
	imlib_context_set_anti_alias(1);

	w = imlib_image_get_width();
	h = imlib_image_get_height();
	zw = (float) THUMB_SIZE / (float) w;
	zh = (float) THUMB_SIZE / (float) h;
	z = MIN(zw, zh);
	t->w = z * w;
	t->h = z * h;

	if (!(t->im = imlib_create_cropped_scaled_image(0, 0, w, h, t->w, t->h)))
		die("could not allocate memory");

	imlib_free_image_and_decache();

	if (use_cache && !cached)
		tns_cache_write(t, False);

	tns->dirty = 1;
	return 1;
}

void tns_check_view(tns_t *tns, Bool scrolled) {
	int r;

	if (!tns)
		return;

	tns->first -= tns->first % tns->cols;
	r = tns->sel % tns->cols;

	if (scrolled) {
		/* move selection into visible area */
		if (tns->sel >= tns->first + tns->cols * tns->rows)
			tns->sel = tns->first + r + tns->cols * (tns->rows - 1);
		else if (tns->sel < tns->first)
			tns->sel = tns->first + r;
	} else {
		/* scroll to selection */
		if (tns->first + tns->cols * tns->rows <= tns->sel) {
			tns->first = tns->sel - r - tns->cols * (tns->rows - 1);
			tns->dirty = 1;
		} else if (tns->first > tns->sel) {
			tns->first = tns->sel - r;
			tns->dirty = 1;
		}
	}
}

void tns_render(tns_t *tns, win_t *win) {
	int i, cnt, r, x, y;
	thumb_t *t;

	if (!tns || !tns->thumbs || !win)
		return;

	if (!tns->dirty)
		return;

	win_clear(win);
	imlib_context_set_drawable(win->pm);

	tns->cols = MAX(1, win->w / thumb_dim);
	tns->rows = MAX(1, win->h / thumb_dim);

	if (tns->cnt < tns->cols * tns->rows) {
		tns->first = 0;
		cnt = tns->cnt;
	} else {
		tns_check_view(tns, False);
		cnt = tns->cols * tns->rows;
		if ((r = tns->first + cnt - tns->cnt) >= tns->cols)
			tns->first -= r - r % tns->cols;
		if (r > 0)
			cnt -= r % tns->cols;
	}

	r = cnt % tns->cols ? 1 : 0;
	tns->x = x = (win->w - MIN(cnt, tns->cols) * thumb_dim) / 2 + 5;
	tns->y = y = (win->h - (cnt / tns->cols + r) * thumb_dim) / 2 + 5;

	for (i = 0; i < cnt; ++i) {
		t = &tns->thumbs[tns->first + i];
		t->x = x + (THUMB_SIZE - t->w) / 2;
		t->y = y + (THUMB_SIZE - t->h) / 2;
		imlib_context_set_image(t->im);
		imlib_render_image_part_on_drawable_at_size(0, 0, t->w, t->h,
		                                            t->x, t->y, t->w, t->h);
		if ((i + 1) % tns->cols == 0) {
			x = tns->x;
			y += thumb_dim;
		} else {
			x += thumb_dim;
		}
	}

	tns->dirty = 0;
	tns_highlight(tns, win, tns->sel, True);
}

void tns_highlight(tns_t *tns, win_t *win, int n, Bool hl) {
	thumb_t *t;
	unsigned long col;

	if (!tns || !tns->thumbs || !win)
		return;

	if (n >= 0 && n < tns->cnt) {
		t = &tns->thumbs[n];

		if (hl)
			col = win->selcol;
		else if (win->fullscreen)
			col = win->black;
		else
			col = win->bgcol;

		win_draw_rect(win, win->pm, t->x - 2, t->y - 2, t->w + 4, t->h + 4,
		              False, 2, col);
	}

	win_draw(win);
}

int tns_move_selection(tns_t *tns, win_t *win, tnsdir_t dir) {
	int old;

	if (!tns || !tns->thumbs || !win)
		return 0;

	old = tns->sel;

	switch (dir) {
		case TNS_LEFT:
			if (tns->sel > 0)
				--tns->sel;
			break;
		case TNS_RIGHT:
			if (tns->sel < tns->cnt - 1)
				++tns->sel;
			break;
		case TNS_UP:
			if (tns->sel >= tns->cols)
				tns->sel -= tns->cols;
			break;
		case TNS_DOWN:
			if (tns->sel + tns->cols < tns->cnt)
				tns->sel += tns->cols;
			break;
	}

	if (tns->sel != old) {
		tns_highlight(tns, win, old, False);
		tns_check_view(tns, False);
		if (!tns->dirty)
			tns_highlight(tns, win, tns->sel, True);
	}

	return tns->sel != old;
}

int tns_scroll(tns_t *tns, tnsdir_t dir) {
	int old;

	if (!tns)
		return 0;

	old = tns->first;

	if (dir == TNS_DOWN && tns->first + tns->cols * tns->rows < tns->cnt) {
		tns->first += tns->cols;
		tns_check_view(tns, True);
		tns->dirty = 1;
	} else if (dir == TNS_UP && tns->first >= tns->cols) {
		tns->first -= tns->cols;
		tns_check_view(tns, True);
		tns->dirty = 1;
	}

	return tns->first != old;
}

int tns_translate(tns_t *tns, int x, int y) {
	int n;
	thumb_t *t;

	if (!tns || !tns->thumbs)
		return -1;

	if (x < tns->x || y < tns->y)
		return -1;

	n = tns->first + (y - tns->y) / thumb_dim * tns->cols +
	    (x - tns->x) / thumb_dim;

	if (n < tns->cnt) {
		t = &tns->thumbs[n];
		if (x >= t->x && x <= t->x + t->w && y >= t->y && y <= t->y + t->h)
			return n;
	}

	return -1;
}
