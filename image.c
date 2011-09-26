/* sxiv: image.c
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

#define _POSIX_C_SOURCE 200112L
#define _FEATURE_CONFIG
#define _IMAGE_CONFIG

#include <string.h>
#include <unistd.h>

#include "image.h"
#include "options.h"
#include "util.h"
#include "config.h"

#if EXIF_SUPPORT
#include <libexif/exif-data.h>
#endif

#if GIF_SUPPORT
#include <stdlib.h>
#include <sys/types.h>
#include <gif_lib.h>
#endif

#define ZOOMDIFF(z1,z2) ((z1) - (z2) > 0.001 || (z1) - (z2) < -0.001)

enum { MIN_GIF_DELAY = 50 };

float zoom_min;
float zoom_max;

void img_init(img_t *img, win_t *win) {
	zoom_min = zoom_levels[0] / 100.0;
	zoom_max = zoom_levels[ARRLEN(zoom_levels) - 1] / 100.0;

	if (!img || !win)
		return;

	imlib_context_set_display(win->env.dpy);
	imlib_context_set_visual(win->env.vis);
	imlib_context_set_colormap(win->env.cmap);

	img->im = NULL;
	img->win = win;
	img->zoom = options->zoom;
	img->zoom = MAX(img->zoom, zoom_min);
	img->zoom = MIN(img->zoom, zoom_max);
	img->checkpan = false;
	img->dirty = false;
	img->aa = options->aa;
	img->alpha = true;
	img->slideshow = false;
	img->ss_delay = SLIDESHOW_DELAY * 1000;
	img->multi.cap = img->multi.cnt = 0;
	img->multi.animate = false;
}

#if EXIF_SUPPORT
void exif_auto_orientate(const fileinfo_t *file) {
	ExifData *ed;
	ExifEntry *entry;
	int byte_order, orientation;

	if (!(ed = exif_data_new_from_file(file->path)))
		return;
	entry = exif_content_get_entry(ed->ifd[EXIF_IFD_0], EXIF_TAG_ORIENTATION);
	if (entry) {
		byte_order = exif_data_get_byte_order(ed);
		orientation = exif_get_short(entry->data, byte_order);
	}
	exif_data_unref(ed);
	if (!entry)
		return;

	switch (orientation) {
		case 5:
			imlib_image_orientate(1);
		case 2:
			imlib_image_flip_vertical();
			break;

		case 3:
			imlib_image_orientate(2);
			break;

		case 7:
			imlib_image_orientate(1);
		case 4:
			imlib_image_flip_horizontal();
			break;

		case 6:
			imlib_image_orientate(1);
			break;

		case 8:
			imlib_image_orientate(3);
			break;
	}
}
#endif /* EXIF_SUPPORT */

#if GIF_SUPPORT
/* Originally based on, but in its current form merely inspired by Imlib2's
 * src/modules/loaders/loader_gif.c:load(), written by Carsten Haitzler.
 */
bool img_load_gif(img_t *img, const fileinfo_t *file) {
	GifFileType *gif;
	GifRowType *rows = NULL;
	GifRecordType rec;
	ColorMapObject *cmap;
	DATA32 bgpixel, *data, *ptr;
	DATA32 *prev_frame = NULL;
	Imlib_Image *im;
	int i, j, bg, r, g, b;
	int x, y, w, h, sw, sh;
	int intoffset[] = { 0, 4, 2, 1 };
	int intjump[] = { 8, 8, 4, 2 };
	int transp = -1;
	unsigned int delay = 0;
	bool err = false;

	if (img->multi.cap == 0) {
		img->multi.cap = 8;
		img->multi.frames = (img_frame_t*)
		                    s_malloc(sizeof(img_frame_t) * img->multi.cap);
	}
	img->multi.cnt = 0;
	img->multi.sel = 0;

	gif = DGifOpenFileName(file->path);
	if (!gif) {
		warn("could not open gif file: %s", file->name);
		return false;
	}
	bg = gif->SBackGroundColor;
	sw = gif->SWidth;
	sh = gif->SHeight;

	do {
		if (DGifGetRecordType(gif, &rec) == GIF_ERROR) {
			err = true;
			break;
		}
		if (rec == EXTENSION_RECORD_TYPE) {
			int ext_code;
			GifByteType *ext = NULL;

			DGifGetExtension(gif, &ext_code, &ext);
			while (ext) {
				if (ext_code == 0xf9) {
					if (ext[1] & 1)
						transp = (int) ext[4];
					else
						transp = -1;

					delay = 10 * ((unsigned int) ext[3] << 8 | (unsigned int) ext[2]);
					if (delay)
						delay = MAX(delay, MIN_GIF_DELAY);
				}
				ext = NULL;
				DGifGetExtensionNext(gif, &ext);
			}
		} else if (rec == IMAGE_DESC_RECORD_TYPE) {
			if (DGifGetImageDesc(gif) == GIF_ERROR) {
				err = true;
				break;
			}
			x = gif->Image.Left;
			y = gif->Image.Top;
			w = gif->Image.Width;
			h = gif->Image.Height;

			rows = (GifRowType*) s_malloc(h * sizeof(GifRowType));
			for (i = 0; i < h; i++)
				rows[i] = (GifRowType) s_malloc(w * sizeof(GifPixelType));
			if (gif->Image.Interlace) {
				for (i = 0; i < 4; i++) {
					for (j = intoffset[i]; j < h; j += intjump[i])
						DGifGetLine(gif, rows[j], w);
				}
			} else {
				for (i = 0; i < h; i++)
					DGifGetLine(gif, rows[i], w);
			}

			ptr = data = (DATA32*) s_malloc(sizeof(DATA32) * sw * sh);
			cmap = gif->Image.ColorMap ? gif->Image.ColorMap : gif->SColorMap;
			r = cmap->Colors[bg].Red;
			g = cmap->Colors[bg].Green;
			b = cmap->Colors[bg].Blue;
			bgpixel = 0x00ffffff & (r << 16 | g << 8 | b);

			for (i = 0; i < sh; i++) {
				for (j = 0; j < sw; j++) {
					if (i < y || i >= y + h || j < x || j >= x + w) {
						if (transp >= 0 && prev_frame)
							*ptr = prev_frame[i * sw + j];
						else
							*ptr = bgpixel;
					} else if (rows[i-y][j-x] == transp) {
						if (prev_frame)
							*ptr = prev_frame[i * sw + j];
						else
							*ptr = bgpixel;
					} else {
						r = cmap->Colors[rows[i-y][j-x]].Red;
						g = cmap->Colors[rows[i-y][j-x]].Green;
						b = cmap->Colors[rows[i-y][j-x]].Blue;
						*ptr = 0xff << 24 | r << 16 | g << 8 | b;
					}
					ptr++;
				}
			}

			im = imlib_create_image_using_copied_data(sw, sh, data);

			for (i = 0; i < h; i++)
				free(rows[i]);
			free(rows);
			free(data);

			if (!im) {
				err = true;
				break;
			}

			imlib_context_set_image(im);
			prev_frame = imlib_image_get_data_for_reading_only();

			imlib_image_set_format("gif");
			if (transp >= 0)
				imlib_image_set_has_alpha(1);

			if (img->multi.cnt == img->multi.cap) {
				img->multi.cap *= 2;
				img->multi.frames = (img_frame_t*)
				                    s_realloc(img->multi.frames,
				                              img->multi.cap * sizeof(img_frame_t));
			}
			img->multi.frames[img->multi.cnt].im = im;
			img->multi.frames[img->multi.cnt].delay = delay ? delay : GIF_DELAY;
			img->multi.cnt++;
		}
	} while (rec != TERMINATE_RECORD_TYPE);

	DGifCloseFile(gif);

	if (err && !file->loaded)
		warn("corrupted gif file: %s", file->name);

	if (img->multi.cnt > 1) {
		imlib_context_set_image(img->im);
		imlib_free_image();
		img->im = img->multi.frames[0].im;
		img->multi.animate = GIF_AUTOPLAY;
	} else if (img->multi.cnt == 1) {
		imlib_context_set_image(img->multi.frames[0].im);
		imlib_free_image();
		img->multi.cnt = 0;
		img->multi.animate = false;
	}

	imlib_context_set_image(img->im);

	return !err;
}
#endif /* GIF_SUPPORT */

bool img_load(img_t *img, const fileinfo_t *file) {
	const char *fmt;

	if (!img || !file || !file->name || !file->path)
		return false;

	if (access(file->path, R_OK) || !(img->im = imlib_load_image(file->path))) {
		warn("could not open image: %s", file->name);
		return false;
	}

	imlib_context_set_image(img->im);
	imlib_image_set_changes_on_disk();
	imlib_context_set_anti_alias(img->aa);

	fmt = imlib_image_format();
	/* avoid unused-but-set-variable warning */
	(void) fmt;

#if EXIF_SUPPORT
	if (STREQ(fmt, "jpeg"))
		exif_auto_orientate(file);
#endif
#if GIF_SUPPORT
	if (STREQ(fmt, "gif"))
		img_load_gif(img, file);
#endif

	img->w = imlib_image_get_width();
	img->h = imlib_image_get_height();
	img->scalemode = options->scalemode;
	img->re = false;
	img->checkpan = false;
	img->dirty = true;

	return true;
}

void img_close(img_t *img, bool decache) {
	int i;

	if (!img)
		return;

	if (img->multi.cnt) {
		for (i = 0; i < img->multi.cnt; i++) {
			imlib_context_set_image(img->multi.frames[i].im);
			imlib_free_image();
		}
		img->multi.cnt = 0;
		img->im = NULL;
	} else if (img->im) {
		imlib_context_set_image(img->im);
		if (decache)
			imlib_free_image_and_decache();
		else
			imlib_free_image();
		img->im = NULL;
	}
}

void img_check_pan(img_t *img, bool moved) {
	win_t *win;
	int ox, oy;

	if (!img || !img->im || !img->win)
		return;

	win = img->win;
	ox = img->x;
	oy = img->y;

	if (img->w * img->zoom > win->w) {
		if (img->x > 0 && img->x + img->w * img->zoom > win->w)
			img->x = 0;
		if (img->x < 0 && img->x + img->w * img->zoom < win->w)
			img->x = win->w - img->w * img->zoom;
	} else {
		img->x = (win->w - img->w * img->zoom) / 2;
	}
	if (img->h * img->zoom > win->h) {
		if (img->y > 0 && img->y + img->h * img->zoom > win->h)
			img->y = 0;
		if (img->y < 0 && img->y + img->h * img->zoom < win->h)
			img->y = win->h - img->h * img->zoom;
	} else {
		img->y = (win->h - img->h * img->zoom) / 2;
	}

	if (!moved && (ox != img->x || oy != img->y))
		img->dirty = true;
}

bool img_fit(img_t *img) {
	float z, zmax, zw, zh;

	if (!img || !img->im || !img->win)
		return false;
	if (img->scalemode == SCALE_ZOOM)
		return false;

	zmax = img->scalemode == SCALE_DOWN ? 1.0 : zoom_max;
	zw = (float) img->win->w / (float) img->w;
	zh = (float) img->win->h / (float) img->h;

	z = MIN(zw, zh);
	z = MAX(z, zoom_min);
	z = MIN(z, zmax);

	if (ZOOMDIFF(z, img->zoom)) {
		img->zoom = z;
		img->dirty = true;
		return true;
	} else {
		return false;
	}
}

void img_render(img_t *img) {
	win_t *win;
	int sx, sy, sw, sh;
	int dx, dy, dw, dh;

	if (!img || !img->im || !img->win)
		return;

	win = img->win;
	img_fit(img);

	if (!img->re) {
		/* rendered for the first time */
		img->re = true;
		if (img->zoom * img->w <= win->w)
			img->x = (win->w - img->w * img->zoom) / 2;
		else
			img->x = 0;
		if (img->zoom * img->h <= win->h)
			img->y = (win->h - img->h * img->zoom) / 2;
		else
			img->y = 0;
	}
	
	if (img->checkpan) {
		img_check_pan(img, false);
		img->checkpan = false;
	}

	if (!img->dirty)
		return;

	/* calculate source and destination offsets */
	if (img->x < 0) {
		sx = -img->x / img->zoom;
		sw = win->w / img->zoom;
		dx = 0;
		dw = win->w;
	} else {
		sx = 0;
		sw = img->w;
		dx = img->x;
		dw = img->w * img->zoom;
	}
	if (img->y < 0) {
		sy = -img->y / img->zoom;
		sh = win->h / img->zoom;
		dy = 0;
		dh = win->h;
	} else {
		sy = 0;
		sh = img->h;
		dy = img->y;
		dh = img->h * img->zoom;
	}

	win_clear(win);

	imlib_context_set_image(img->im);

	if (imlib_image_has_alpha() && !img->alpha)
		win_draw_rect(win, win->pm, dx, dy, dw, dh, True, 0, win->white);
	
	imlib_context_set_drawable(win->pm);
	imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw, dh);

	win_draw(win);

	img->dirty = false;
}

bool img_fit_win(img_t *img) {
	if (!img || !img->im)
		return false;

	img->scalemode = SCALE_FIT;
	return img_fit(img);
}

bool img_center(img_t *img) {
	int ox, oy;

	if (!img || !img->im || !img->win)
		return false;
	
	ox = img->x;
	oy = img->y;

	img->x = (img->win->w - img->w * img->zoom) / 2;
	img->y = (img->win->h - img->h * img->zoom) / 2;
	
	if (ox != img->x || oy != img->y) {
		img->dirty = true;
		return true;
	} else {
		return false;
	}
}

bool img_zoom(img_t *img, float z) {
	if (!img || !img->im || !img->win)
		return false;

	z = MAX(z, zoom_min);
	z = MIN(z, zoom_max);

	img->scalemode = SCALE_ZOOM;

	if (ZOOMDIFF(z, img->zoom)) {
		img->x = img->win->w / 2 - (img->win->w / 2 - img->x) * z / img->zoom;
		img->y = img->win->h / 2 - (img->win->h / 2 - img->y) * z / img->zoom;
		img->zoom = z;
		img->checkpan = true;
		img->dirty = true;
		return true;
	} else {
		return false;
	}
}

bool img_zoom_in(img_t *img) {
	int i;

	if (!img || !img->im)
		return false;

	for (i = 1; i < ARRLEN(zoom_levels); i++) {
		if (zoom_levels[i] > img->zoom * 100.0)
			return img_zoom(img, zoom_levels[i] / 100.0);
	}
	return false;
}

bool img_zoom_out(img_t *img) {
	int i;

	if (!img || !img->im)
		return false;

	for (i = ARRLEN(zoom_levels) - 2; i >= 0; i--) {
		if (zoom_levels[i] < img->zoom * 100.0)
			return img_zoom(img, zoom_levels[i] / 100.0);
	}
	return false;
}

bool img_move(img_t *img, int dx, int dy) {
	int ox, oy;

	if (!img || !img->im)
		return false;

	ox = img->x;
	oy = img->y;

	img->x += dx;
	img->y += dy;

	img_check_pan(img, true);

	if (ox != img->x || oy != img->y) {
		img->dirty = true;
		return true;
	} else {
		return false;
	}
}

bool img_pan(img_t *img, direction_t dir, bool screen) {
	if (!img || !img->im || !img->win)
		return false;

	switch (dir) {
		case DIR_LEFT:
			return img_move(img, img->win->w / (screen ? 1 : 5), 0);
		case DIR_RIGHT:
			return img_move(img, img->win->w / (screen ? 1 : 5) * -1, 0);
		case DIR_UP:
			return img_move(img, 0, img->win->h / (screen ? 1 : 5));
		case DIR_DOWN:
			return img_move(img, 0, img->win->h / (screen ? 1 : 5) * -1);
	}
	return false;
}

bool img_pan_edge(img_t *img, direction_t dir) {
	int ox, oy;

	if (!img || !img->im || !img->win)
		return false;

	ox = img->x;
	oy = img->y;

	switch (dir) {
		case DIR_LEFT:
			img->x = 0;
			break;
		case DIR_RIGHT:
			img->x = img->win->w - img->w * img->zoom;
			break;
		case DIR_UP:
			img->y = 0;
			break;
		case DIR_DOWN:
			img->y = img->win->h - img->h * img->zoom;
			break;
	}

	img_check_pan(img, true);

	if (ox != img->x || oy != img->y) {
		img->dirty = true;
		return true;
	} else {
		return false;
	}
}

void img_rotate(img_t *img, int d) {
	win_t *win;
	int ox, oy, tmp;

	if (!img || !img->im || !img->win)
		return;

	win = img->win;
	ox = d == 1 ? img->x : win->w - img->x - img->w * img->zoom;
	oy = d == 3 ? img->y : win->h - img->y - img->h * img->zoom;

	imlib_context_set_image(img->im);
	imlib_image_orientate(d);

	img->x = oy + (win->w - win->h) / 2;
	img->y = ox + (win->h - win->w) / 2;

	tmp = img->w;
	img->w = img->h;
	img->h = tmp;

	img->checkpan = true;
	img->dirty = true;
}

void img_rotate_left(img_t *img) {
	img_rotate(img, 3);
}

void img_rotate_right(img_t *img) {
	img_rotate(img, 1);
}

void img_toggle_antialias(img_t *img) {
	if (!img || !img->im)
		return;

	img->aa = !img->aa;
	imlib_context_set_image(img->im);
	imlib_context_set_anti_alias(img->aa);
	img->dirty = true;
}

bool img_frame_goto(img_t *img, int n) {
	if (!img || !img->im)
		return false;
	if (n < 0 || n >= img->multi.cnt || n == img->multi.sel)
		return false;

	img->multi.sel = n;
	img->im = img->multi.frames[n].im;

	imlib_context_set_image(img->im);
	img->w = imlib_image_get_width();
	img->h = imlib_image_get_height();
	img->checkpan = true;
	img->dirty = true;

	return true;
}

bool img_frame_navigate(img_t *img, int d) {
	if (!img || !img->im || !img->multi.cnt || !d)
		return false;

	d += img->multi.sel;
	if (d < 0)
		d = 0;
	else if (d >= img->multi.cnt)
		d = img->multi.cnt - 1;

	return img_frame_goto(img, d);
}

bool img_frame_animate(img_t *img, bool restart) {
	if (!img || !img->im || !img->multi.cnt)
		return false;

	if (img->multi.sel + 1 >= img->multi.cnt) {
		if (restart || (GIF_LOOP && !img->slideshow)) {
			img_frame_goto(img, 0);
		} else {
			img->multi.animate = false;
			return false;
		}
	} else if (!restart) {
		img_frame_goto(img, img->multi.sel + 1);
	}
	img->multi.animate = true;
	img->dirty = true;

	return true;
}
