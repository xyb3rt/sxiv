/* sxiv: image.c
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
#define _IMAGE_CONFIG

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <gif_lib.h>

#include "exif.h"
#include "image.h"
#include "options.h"
#include "util.h"
#include "config.h"

enum { MIN_GIF_DELAY = 50 };

float zoom_min;
float zoom_max;

int zoomdiff(float z1, float z2) {
	return (int) (z1 * 1000.0 - z2 * 1000.0);
}

void img_init(img_t *img, win_t *win) {
	zoom_min = zoom_levels[0] / 100.0;
	zoom_max = zoom_levels[ARRLEN(zoom_levels) - 1] / 100.0;

	if (img == NULL || win == NULL)
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
	img->multi.cap = img->multi.cnt = 0;
	img->multi.animate = false;
}

void exif_auto_orientate(const fileinfo_t *file) {
	switch (exif_orientation(file)) {
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
	int px, py, pw, ph;
	int intoffset[] = { 0, 4, 2, 1 };
	int intjump[] = { 8, 8, 4, 2 };
	int transp = -1;
	unsigned int disposal = 0, prev_disposal = 0;
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
	if (gif == NULL) {
		warn("could not open gif file: %s", file->name);
		return false;
	}
	bg = gif->SBackGroundColor;
	sw = gif->SWidth;
	sh = gif->SHeight;
	px = py = pw = ph = 0;

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

					disposal = (unsigned int) ext[1] >> 2 & 0x7;
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
					if (i < y || i >= y + h || j < x || j >= x + w ||
					    rows[i-y][j-x] == transp)
					{
						if (prev_frame != NULL && (prev_disposal != 2 ||
						    i < py || i >= py + ph || j < px || j >= px + pw))
						{
							*ptr = prev_frame[i * sw + j];
						} else {
							*ptr = bgpixel;
						}
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

			if (im == NULL) {
				err = true;
				break;
			}

			imlib_context_set_image(im);
			imlib_image_set_format("gif");
			if (transp >= 0)
				imlib_image_set_has_alpha(1);

			if (disposal != 3)
				prev_frame = imlib_image_get_data_for_reading_only();
			prev_disposal = disposal;
			px = x, py = y, pw = w, ph = h;

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

bool img_load(img_t *img, const fileinfo_t *file) {
	const char *fmt;

	if (img == NULL || file == NULL || file->name == NULL || file->path == NULL)
		return false;

	if (access(file->path, R_OK) < 0 ||
	    (img->im = imlib_load_image(file->path)) == NULL)
	{
		warn("could not open image: %s", file->name);
		return false;
	}

	imlib_context_set_image(img->im);
	imlib_image_set_changes_on_disk();

	if ((fmt = imlib_image_format()) == NULL) {
		warn("could not open image: %s", file->name);
		return false;
	}
	if (STREQ(fmt, "jpeg"))
		exif_auto_orientate(file);
	if (STREQ(fmt, "gif"))
		img_load_gif(img, file);

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

	if (img == NULL)
		return;

	if (img->multi.cnt > 0) {
		for (i = 0; i < img->multi.cnt; i++) {
			imlib_context_set_image(img->multi.frames[i].im);
			imlib_free_image();
		}
		img->multi.cnt = 0;
		img->im = NULL;
	} else if (img->im != NULL) {
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

	if (img == NULL || img->im == NULL || img->win == NULL)
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

	if (img == NULL || img->im == NULL || img->win == NULL)
		return false;
	if (img->scalemode == SCALE_ZOOM)
		return false;

	zmax = img->scalemode == SCALE_DOWN ? 1.0 : zoom_max;
	zw = (float) img->win->w / (float) img->w;
	zh = (float) img->win->h / (float) img->h;

	switch (img->scalemode) {
		case SCALE_WIDTH:
			z = zw;
			break;
		case SCALE_HEIGHT:
			z = zh;
			break;
		default:
			z = MIN(zw, zh);
			break;
	}

	z = MAX(z, zoom_min);
	z = MIN(z, zmax);

	if (zoomdiff(z, img->zoom) != 0) {
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

	if (img == NULL || img->im == NULL || img->win == NULL)
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
	imlib_context_set_anti_alias(img->aa);

	if (!img->alpha && imlib_image_has_alpha())
		win_draw_rect(win, win->pm, dx, dy, dw, dh, True, 0, win->white);
	
	imlib_context_set_drawable(win->pm);
	imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw, dh);

	img->dirty = false;
}

bool img_fit_win(img_t *img, scalemode_t sm) {
	if (img == NULL || img->im == NULL)
		return false;

	img->scalemode = sm;
	return img_fit(img);
}

bool img_center(img_t *img) {
	int ox, oy;

	if (img == NULL || img->im == NULL || img->win == NULL)
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
	if (img == NULL || img->im == NULL || img->win == NULL)
		return false;

	z = MAX(z, zoom_min);
	z = MIN(z, zoom_max);

	img->scalemode = SCALE_ZOOM;

	if (zoomdiff(z, img->zoom) != 0) {
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
	float z;

	if (img == NULL || img->im == NULL)
		return false;

	for (i = 1; i < ARRLEN(zoom_levels); i++) {
		z = zoom_levels[i] / 100.0;
		if (zoomdiff(z, img->zoom) > 0)
			return img_zoom(img, z);
	}
	return false;
}

bool img_zoom_out(img_t *img) {
	int i;
	float z;

	if (img == NULL || img->im == NULL)
		return false;

	for (i = ARRLEN(zoom_levels) - 2; i >= 0; i--) {
		z = zoom_levels[i] / 100.0;
		if (zoomdiff(z, img->zoom) < 0)
			return img_zoom(img, z);
	}
	return false;
}

bool img_move(img_t *img, float dx, float dy) {
	float ox, oy;

	if (img == NULL || img->im == NULL)
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

bool img_pan(img_t *img, direction_t dir, int d) {
	/* d < 0: screen-wise
	 * d = 0: 1/5 of screen
	 * d > 0: num of pixels
	 */
	float x, y;

	if (img == NULL || img->im == NULL || img->win == NULL)
		return false;

	if (d > 0) {
		x = y = MAX(1, (float) d * img->zoom);
	} else {
		x = img->win->w / (d < 0 ? 1 : 5);
		y = img->win->h / (d < 0 ? 1 : 5);
	}

	switch (dir) {
		case DIR_LEFT:
			return img_move(img, x, 0.0);
		case DIR_RIGHT:
			return img_move(img, -x, 0.0);
		case DIR_UP:
			return img_move(img, 0.0, y);
		case DIR_DOWN:
			return img_move(img, 0.0, -y);
	}
	return false;
}

bool img_pan_edge(img_t *img, direction_t dir) {
	int ox, oy;

	if (img == NULL || img->im == NULL || img->win == NULL)
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

	if (img == NULL || img->im == NULL || img->win == NULL)
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

void img_flip(img_t *img, flipdir_t d) {
	if (img == NULL || img->im == NULL)
		return;

	imlib_context_set_image(img->im);

	switch (d) {
		case FLIP_HORIZONTAL:
			imlib_image_flip_horizontal();
			break;
		case FLIP_VERTICAL:
			imlib_image_flip_vertical();
			break;
	}
	img->dirty = true;
}

void img_toggle_antialias(img_t *img) {
	if (img == NULL || img->im == NULL)
		return;

	img->aa = !img->aa;
	imlib_context_set_image(img->im);
	imlib_context_set_anti_alias(img->aa);
	img->dirty = true;
}

bool img_frame_goto(img_t *img, int n) {
	if (img == NULL || img->im == NULL)
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
	if (img == NULL|| img->im == NULL || img->multi.cnt == 0 || d == 0)
		return false;

	d += img->multi.sel;
	if (d < 0)
		d = 0;
	else if (d >= img->multi.cnt)
		d = img->multi.cnt - 1;

	return img_frame_goto(img, d);
}

bool img_frame_animate(img_t *img, bool restart) {
	if (img == NULL || img->im == NULL || img->multi.cnt == 0)
		return false;

	if (img->multi.sel + 1 >= img->multi.cnt) {
		if (restart || GIF_LOOP) {
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
