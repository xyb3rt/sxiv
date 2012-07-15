/* sxiv: thumbs.c
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
#define _THUMBS_CONFIG

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

#include "exif.h"
#include "thumbs.h"
#include "util.h"
#include "config.h"

const int thumb_dim = THUMB_SIZE + 10;
char *cache_dir = NULL;

bool tns_cache_enabled(void) {
	struct stat stats;

	return cache_dir != NULL && stat(cache_dir, &stats) == 0 &&
	       S_ISDIR(stats.st_mode) && access(cache_dir, W_OK) == 0;
}

char* tns_cache_filepath(const char *filepath) {
	size_t len;
	char *cfile = NULL;

	if (cache_dir == NULL || filepath == NULL || *filepath != '/')
		return NULL;
	
	if (strncmp(filepath, cache_dir, strlen(cache_dir)) != 0) {
		/* don't cache images inside the cache directory! */
		len = strlen(cache_dir) + strlen(filepath) + 6;
		cfile = (char*) s_malloc(len);
		snprintf(cfile, len, "%s/%s.png", cache_dir, filepath + 1);
	}
	return cfile;
}

Imlib_Image* tns_cache_load(const char *filepath) {
	char *cfile;
	struct stat cstats, fstats;
	Imlib_Image *im = NULL;

	if (filepath == NULL)
		return NULL;
	if (stat(filepath, &fstats) < 0)
		return NULL;

	if ((cfile = tns_cache_filepath(filepath)) != NULL) {
		if (stat(cfile, &cstats) == 0 && cstats.st_mtime == fstats.st_mtime)
			im = imlib_load_image(cfile);
		free(cfile);
	}
	return im;
}

void tns_cache_write(thumb_t *t, bool force) {
	char *cfile, *dirend;
	struct stat cstats, fstats;
	struct utimbuf times;
	Imlib_Load_Error err = 0;

	if (t == NULL || t->im == NULL)
		return;
	if (t->file == NULL || t->file->name == NULL || t->file->path == NULL)
		return;
	if (stat(t->file->path, &fstats) < 0)
		return;

	if ((cfile = tns_cache_filepath(t->file->path)) != NULL) {
		if (force || stat(cfile, &cstats) < 0 ||
		    cstats.st_mtime != fstats.st_mtime)
		{
			if ((dirend = strrchr(cfile, '/')) != NULL) {
				*dirend = '\0';
				err = r_mkdir(cfile);
				*dirend = '/';
			}

			if (err == 0) {
				imlib_context_set_image(t->im);
				imlib_image_set_format("png");
				imlib_save_image_with_error_return(cfile, &err);
			}

			if (err == 0) {
				times.actime = fstats.st_atime;
				times.modtime = fstats.st_mtime;
				utime(cfile, &times);
			} else {
				warn("could not cache thumbnail: %s", t->file->name);
			}
		}
		free(cfile);
	}
}

void tns_clean_cache(tns_t *tns) {
	int dirlen;
	bool delete;
	char *cfile, *filename, *tpos;
	r_dir_t dir;

	if (cache_dir == NULL)
		return;
	
	if (r_opendir(&dir, cache_dir) < 0) {
		warn("could not open thumbnail cache directory: %s", cache_dir);
		return;
	}

	dirlen = strlen(cache_dir);

	while ((cfile = r_readdir(&dir)) != NULL) {
		filename = cfile + dirlen;
		delete = false;

		if ((tpos = strrchr(filename, '.')) != NULL) {
			*tpos = '\0';
			if (access(filename, F_OK) < 0)
				delete = true;
			*tpos = '.';
		}
		if (delete) {
			if (unlink(cfile) < 0)
				warn("could not delete cache file: %s", cfile);
		}
		free(cfile);
	}
	r_closedir(&dir);
}


void tns_init(tns_t *tns, int cnt, win_t *win) {
	int len;
	char *homedir;

	if (tns == NULL)
		return;

	if (cnt > 0) {
		tns->thumbs = (thumb_t*) s_malloc(cnt * sizeof(thumb_t));
		memset(tns->thumbs, 0, cnt * sizeof(thumb_t));
	} else {
		tns->thumbs = NULL;
	}

	tns->cap = cnt;
	tns->cnt = tns->first = tns->sel = 0;
	tns->win = win;
	tns->alpha = true;
	tns->dirty = false;

	if ((homedir = getenv("HOME")) != NULL) {
		if (cache_dir != NULL)
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

	if (tns == NULL)
		return;

	if (tns->thumbs != NULL) {
		for (i = 0; i < tns->cnt; i++) {
			if (tns->thumbs[i].im != NULL) {
				imlib_context_set_image(tns->thumbs[i].im);
				imlib_free_image();
			}
		}
		free(tns->thumbs);
		tns->thumbs = NULL;
	}

	if (cache_dir != NULL) {
		free(cache_dir);
		cache_dir = NULL;
	}
}

bool tns_load(tns_t *tns, int n, const fileinfo_t *file,
              bool force, bool silent)
{
	int w, h;
	bool use_cache, cache_hit = false;
	float z, zw, zh;
	thumb_t *t;
	Imlib_Image *im;
	const char *fmt;

	if (tns == NULL || tns->thumbs == NULL)
		return false;
	if (file == NULL || file->name == NULL || file->path == NULL)
		return false;
	if (n < 0 || n >= tns->cap)
		return false;

	t = &tns->thumbs[n];
	t->file = file;

	if (t->im != NULL) {
		imlib_context_set_image(t->im);
		imlib_free_image();
	}

	if ((use_cache = tns_cache_enabled())) {
		if (!force && (im = tns_cache_load(file->path)) != NULL)
			cache_hit = true;
	}

	if (!cache_hit) {
		if (access(file->path, R_OK) < 0 ||
		    (im = imlib_load_image(file->path)) == NULL)
		{
			if (!silent)
				warn("could not open image: %s", file->name);
			return false;
		}
	}

	imlib_context_set_image(im);
	imlib_context_set_anti_alias(1);

	if ((fmt = imlib_image_format()) == NULL) {
		if (!silent)
			warn("could not open image: %s", file->name);
		imlib_free_image_and_decache();
		return false;
	}
	if (STREQ(fmt, "jpeg"))
		exif_auto_orientate(file);

	w = imlib_image_get_width();
	h = imlib_image_get_height();
	zw = (float) THUMB_SIZE / (float) w;
	zh = (float) THUMB_SIZE / (float) h;
	z = MIN(zw, zh);
	z = MIN(z, 1.0);
	t->w = z * w;
	t->h = z * h;

	t->im = imlib_create_cropped_scaled_image(0, 0, w, h, t->w, t->h);
	if (t->im == NULL)
		die("could not allocate memory");

	imlib_free_image_and_decache();

	if (use_cache && !cache_hit)
		tns_cache_write(t, true);

	tns->dirty = true;
	return true;
}

void tns_check_view(tns_t *tns, bool scrolled) {
	int r;

	if (tns == NULL)
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
			tns->dirty = true;
		} else if (tns->first > tns->sel) {
			tns->first = tns->sel - r;
			tns->dirty = true;
		}
	}
}

void tns_render(tns_t *tns) {
	thumb_t *t;
	win_t *win;
	int i, cnt, r, x, y;

	if (tns == NULL || tns->thumbs == NULL || tns->win == NULL)
		return;
	if (!tns->dirty)
		return;

	win = tns->win;
	win_clear(win);
	imlib_context_set_drawable(win->pm);

	tns->cols = MAX(1, win->w / thumb_dim);
	tns->rows = MAX(1, win->h / thumb_dim);

	if (tns->cnt < tns->cols * tns->rows) {
		tns->first = 0;
		cnt = tns->cnt;
	} else {
		tns_check_view(tns, false);
		cnt = tns->cols * tns->rows;
		if ((r = tns->first + cnt - tns->cnt) >= tns->cols)
			tns->first -= r - r % tns->cols;
		if (r > 0)
			cnt -= r % tns->cols;
	}

	r = cnt % tns->cols ? 1 : 0;
	tns->x = x = (win->w - MIN(cnt, tns->cols) * thumb_dim) / 2 + 5;
	tns->y = y = (win->h - (cnt / tns->cols + r) * thumb_dim) / 2 + 5;

	for (i = 0; i < cnt; i++) {
		t = &tns->thumbs[tns->first + i];
		t->x = x + (THUMB_SIZE - t->w) / 2;
		t->y = y + (THUMB_SIZE - t->h) / 2;
		imlib_context_set_image(t->im);

		if (!tns->alpha && imlib_image_has_alpha())
			win_draw_rect(win, win->pm, t->x, t->y, t->w, t->h, true, 0, win->white);

		imlib_render_image_part_on_drawable_at_size(0, 0, t->w, t->h,
		                                            t->x, t->y, t->w, t->h);
		if ((i + 1) % tns->cols == 0) {
			x = tns->x;
			y += thumb_dim;
		} else {
			x += thumb_dim;
		}
	}
	tns->dirty = false;
	tns_highlight(tns, tns->sel, true);
}

void tns_highlight(tns_t *tns, int n, bool hl) {
	thumb_t *t;
	win_t *win;
	int x, y;
	unsigned long col;

	if (tns == NULL || tns->thumbs == NULL || tns->win == NULL)
		return;

	win = tns->win;

	if (n >= 0 && n < tns->cnt) {
		t = &tns->thumbs[n];

		if (hl)
			col = win->selcol;
		else if (win->fullscreen)
			col = win->fscol;
		else
			col = win->bgcol;

		x = t->x - (THUMB_SIZE - t->w) / 2;
		y = t->y - (THUMB_SIZE - t->h) / 2;
		win_draw_rect(win, win->pm, x - 3, y - 3, THUMB_SIZE + 6, THUMB_SIZE + 6,
		              false, 2, col);
	}
}

bool tns_move_selection(tns_t *tns, direction_t dir, int cnt) {
	int old, max;

	if (tns == NULL || tns->thumbs == NULL)
		return false;

	old = tns->sel;
	cnt = cnt > 1 ? cnt : 1;

	switch (dir) {
		case DIR_UP:
			tns->sel = MAX(tns->sel - cnt * tns->cols, tns->sel % tns->cols);
			break;
		case DIR_DOWN:
			max = tns->cols * ((tns->cnt - 1) / tns->cols) +
			      MIN((tns->cnt - 1) % tns->cols, tns->sel % tns->cols);
			tns->sel = MIN(tns->sel + cnt * tns->cols, max);
			break;
		case DIR_LEFT:
			tns->sel = MAX(tns->sel - cnt, 0);
			break;
		case DIR_RIGHT:
			tns->sel = MIN(tns->sel + cnt, tns->cnt - 1);
			break;
	}

	if (tns->sel != old) {
		tns_highlight(tns, old, false);
		tns_check_view(tns, false);
		if (!tns->dirty)
			tns_highlight(tns, tns->sel, true);
	}
	return tns->sel != old;
}

bool tns_scroll(tns_t *tns, direction_t dir, bool screen) {
	int d, max, old;

	if (tns == NULL)
		return false;

	old = tns->first;
	d = tns->cols * (screen ? tns->rows : 1);

	if (dir == DIR_DOWN) {
		max = tns->cnt - tns->cols * tns->rows;
		if (tns->cnt % tns->cols != 0)
			max += tns->cols - tns->cnt % tns->cols;
		tns->first = MIN(tns->first + d, max);
	} else if (dir == DIR_UP) {
		tns->first = MAX(tns->first - d, 0);
	}

	if (tns->first != old) {
		tns_check_view(tns, true);
		tns->dirty = true;
	}
	return tns->first != old;
}

int tns_translate(tns_t *tns, int x, int y) {
	int n;

	if (tns == NULL || tns->thumbs == NULL)
		return -1;
	if (x < tns->x || y < tns->y)
		return -1;

	n = tns->first + (y - tns->y) / thumb_dim * tns->cols +
	    (x - tns->x) / thumb_dim;
	if (n >= tns->cnt)
		n = -1;

	return n;
}
