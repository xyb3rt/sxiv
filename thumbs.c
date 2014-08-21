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

#define _POSIX_C_SOURCE 200112L
#define _THUMBS_CONFIG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

#include "thumbs.h"
#include "util.h"
#include "config.h"

#if HAVE_LIBEXIF
#include <libexif/exif-data.h>
void exif_auto_orientate(const fileinfo_t*);
#endif

static char *cache_dir;
static const int thumb_dim = THUMB_SIZE + 10;

char* tns_cache_filepath(const char *filepath)
{
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

Imlib_Image tns_cache_load(const char *filepath)
{
	char *cfile;
	struct stat cstats, fstats;
	Imlib_Image im = NULL;

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

void tns_cache_write(thumb_t *t, const fileinfo_t *file, bool force)
{
	char *cfile, *dirend;
	struct stat cstats, fstats;
	struct utimbuf times;
	Imlib_Load_Error err = 0;

	if (t == NULL || t->im == NULL)
		return;
	if (file == NULL || file->name == NULL || file->path == NULL)
		return;
	if (stat(file->path, &fstats) < 0)
		return;

	if ((cfile = tns_cache_filepath(file->path)) != NULL) {
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
			}
		}
		free(cfile);
	}
}

void tns_clean_cache(tns_t *tns)
{
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


void tns_init(tns_t *tns, const fileinfo_t *files, int cnt, int *sel, win_t *win)
{
	int len;
	const char *homedir, *dsuffix = "";

	if (tns == NULL)
		return;

	if (cnt > 0) {
		tns->thumbs = (thumb_t*) s_malloc(cnt * sizeof(thumb_t));
		memset(tns->thumbs, 0, cnt * sizeof(thumb_t));
	} else {
		tns->thumbs = NULL;
	}
	tns->files = files;
	tns->cap = cnt;
	tns->cnt = tns->loadnext = tns->first = 0;
	tns->sel = sel;
	tns->win = win;
	tns->dirty = false;

	if ((homedir = getenv("XDG_CACHE_HOME")) == NULL || homedir[0] == '\0') {
		homedir = getenv("HOME");
		dsuffix = "/.cache";
	}
	if (homedir != NULL) {
		free(cache_dir);
		len = strlen(homedir) + strlen(dsuffix) + 6;
		cache_dir = (char*) s_malloc(len);
		snprintf(cache_dir, len, "%s%s/sxiv", homedir, dsuffix);
	} else {
		warn("could not locate thumbnail cache directory");
	}
}

void tns_free(tns_t *tns)
{
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

	free(cache_dir);
	cache_dir = NULL;
}

bool tns_load(tns_t *tns, int n, bool force)
{
	int w, h;
	bool cache_hit = false;
	float z, zw, zh;
	thumb_t *t;
	Imlib_Image im = NULL;
	const fileinfo_t *file;

	if (tns == NULL || tns->thumbs == NULL)
		return false;
	if (n < 0 || n >= tns->cap)
		return false;
	file = &tns->files[n];
	if (file->name == NULL || file->path == NULL)
		return false;

	t = &tns->thumbs[n];

	if (t->im != NULL) {
		imlib_context_set_image(t->im);
		imlib_free_image();
	}

	if (!force && (im = tns_cache_load(file->path)) != NULL) {
		cache_hit = true;
	} else {
#if HAVE_LIBEXIF
		if (!force) {
			int pw = 0, ph = 0, x = 0, y = 0;
			bool err;
			ExifData *ed;
			ExifEntry *entry;
			ExifContent *ifd;
			ExifByteOrder byte_order;
			int tmpfd;
			char tmppath[] = "/tmp/sxiv-XXXXXX";
			Imlib_Image tmpim;

			if ((ed = exif_data_new_from_file(file->path)) != NULL &&
					ed->data != NULL && ed->size > 0)
			{
				if ((tmpfd = mkstemp(tmppath)) >= 0) {
					err = write(tmpfd, ed->data, ed->size) != ed->size;
					close(tmpfd);

					if (!err && (tmpim = imlib_load_image(tmppath)) != NULL) {
						byte_order = exif_data_get_byte_order(ed);
						ifd = ed->ifd[EXIF_IFD_EXIF];
						entry = exif_content_get_entry(ifd, EXIF_TAG_PIXEL_X_DIMENSION);
						if (entry != NULL)
							pw = exif_get_long(entry->data, byte_order);
						entry = exif_content_get_entry(ifd, EXIF_TAG_PIXEL_Y_DIMENSION);
						if (entry != NULL)
							ph = exif_get_long(entry->data, byte_order);

						imlib_context_set_image(tmpim);
						w = imlib_image_get_width();
						h = imlib_image_get_height();

						if (pw > w && ph > h && (pw - ph >= 0) == (w - h >= 0)) {
							zw = (float) pw / (float) w;
							zh = (float) ph / (float) h;
							if (zw < zh) {
								pw /= zh;
								x = (w - pw) / 2;
								w = pw;
							} else if (zw > zh) {
								ph /= zw;
								y = (h - ph) / 2;
								h = ph;
							}
						}
						if ((im = imlib_create_cropped_image(x, y, w, h)) == NULL)
							die("could not allocate memory");
						imlib_free_image_and_decache();
					}
					unlink(tmppath);
				}
				exif_data_unref(ed);
			}
		}
#endif
		if (im == NULL && (access(file->path, R_OK) < 0 ||
		    (im = imlib_load_image(file->path)) == NULL))
		{
			warn("could not open image: %s", file->name);
			return false;
		}
	}
	imlib_context_set_image(im);
	imlib_context_set_anti_alias(1);

#if HAVE_LIBEXIF
	if (!cache_hit)
		exif_auto_orientate(file);
#endif

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

	if (!cache_hit)
		tns_cache_write(t, file, true);

	t->loaded = true;
	tns->dirty = true;
	return true;
}

void tns_check_view(tns_t *tns, bool scrolled)
{
	int r;

	if (tns == NULL)
		return;

	tns->first -= tns->first % tns->cols;
	r = *tns->sel % tns->cols;

	if (scrolled) {
		/* move selection into visible area */
		if (*tns->sel >= tns->first + tns->cols * tns->rows)
			*tns->sel = tns->first + r + tns->cols * (tns->rows - 1);
		else if (*tns->sel < tns->first)
			*tns->sel = tns->first + r;
	} else {
		/* scroll to selection */
		if (tns->first + tns->cols * tns->rows <= *tns->sel) {
			tns->first = *tns->sel - r - tns->cols * (tns->rows - 1);
			tns->dirty = true;
		} else if (tns->first > *tns->sel) {
			tns->first = *tns->sel - r;
			tns->dirty = true;
		}
	}
}

void tns_render(tns_t *tns)
{
	thumb_t *t;
	win_t *win;
	int i, cnt, r, x, y;

	if (tns == NULL || tns->thumbs == NULL || tns->win == NULL)
		return;
	if (!tns->dirty)
		return;

	win = tns->win;
	win_clear(win);
	imlib_context_set_drawable(win->buf.pm);

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
		imlib_render_image_part_on_drawable_at_size(0, 0, t->w, t->h,
		                                            t->x, t->y, t->w, t->h);
		if (tns->files[tns->first + i].marked)
			tns_mark(tns, tns->first + i, true);
		if ((i + 1) % tns->cols == 0) {
			x = tns->x;
			y += thumb_dim;
		} else {
			x += thumb_dim;
		}
	}
	tns->dirty = false;
	tns_highlight(tns, *tns->sel, true);
}

void tns_mark(tns_t *tns, int n, bool mark)
{
	if (tns == NULL || tns->thumbs == NULL || tns->win == NULL)
		return;

	if (n >= 0 && n < tns->cnt) {
		win_t *win = tns->win;
		thumb_t *t = &tns->thumbs[n];
		unsigned long col = win->fullscreen ? win->fscol : win->bgcol;
		int x = t->x + t->w, y = t->y + t->h;

		win_draw_rect(win, x - 2, y + 1, 1, 2, true, 1, col);
		win_draw_rect(win, x + 1, y - 2, 2, 1, true, 1, col);

		if (mark)
			col = win->selcol;

		win_draw_rect(win, x - 1, y + 1, 6, 2, true, 1, col);
		win_draw_rect(win, x + 1, y - 1, 2, 6, true, 1, col);

		if (!mark && n == *tns->sel)
			tns_highlight(tns, n, true);
	}
}

void tns_highlight(tns_t *tns, int n, bool hl)
{
	if (tns == NULL || tns->thumbs == NULL || tns->win == NULL)
		return;

	if (n >= 0 && n < tns->cnt) {
		win_t *win = tns->win;
		thumb_t *t = &tns->thumbs[n];
		unsigned long col;

		if (hl)
			col = win->selcol;
		else
			col = win->fullscreen ? win->fscol : win->bgcol;

		win_draw_rect(win, t->x - 2, t->y - 2, t->w + 4, t->h + 4, false, 2, col);

		if (tns->files[n].marked)
			tns_mark(tns, n, true);
	}
}

bool tns_move_selection(tns_t *tns, direction_t dir, int cnt)
{
	int old, max;

	if (tns == NULL || tns->thumbs == NULL)
		return false;

	old = *tns->sel;
	cnt = cnt > 1 ? cnt : 1;

	switch (dir) {
		case DIR_UP:
			*tns->sel = MAX(*tns->sel - cnt * tns->cols, *tns->sel % tns->cols);
			break;
		case DIR_DOWN:
			max = tns->cols * ((tns->cnt - 1) / tns->cols) +
			      MIN((tns->cnt - 1) % tns->cols, *tns->sel % tns->cols);
			*tns->sel = MIN(*tns->sel + cnt * tns->cols, max);
			break;
		case DIR_LEFT:
			*tns->sel = MAX(*tns->sel - cnt, 0);
			break;
		case DIR_RIGHT:
			*tns->sel = MIN(*tns->sel + cnt, tns->cnt - 1);
			break;
	}

	if (*tns->sel != old) {
		tns_highlight(tns, old, false);
		tns_check_view(tns, false);
		if (!tns->dirty)
			tns_highlight(tns, *tns->sel, true);
	}
	return *tns->sel != old;
}

bool tns_scroll(tns_t *tns, direction_t dir, bool screen)
{
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

int tns_translate(tns_t *tns, int x, int y)
{
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
