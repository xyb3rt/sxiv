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

#include <unistd.h>

#ifdef GIF_SUPPORT
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <gif_lib.h>
#endif

#include "image.h"
#include "options.h"
#include "util.h"

#define _IMAGE_CONFIG
#include "config.h"

enum { MIN_GIF_DELAY = 50 };

float zoom_min;
float zoom_max;

void img_init(img_t *img, win_t *win) {
	zoom_min = zoom_levels[0] / 100.0;
	zoom_max = zoom_levels[ARRLEN(zoom_levels) - 1] / 100.0;

	if (img) {
		img->im = NULL;
		img->multi.cap = img->multi.cnt = 0;
		img->multi.animate = 0;
		img->zoom = options->zoom;
		img->zoom = MAX(img->zoom, zoom_min);
		img->zoom = MIN(img->zoom, zoom_max);
		img->aa = options->aa;
		img->alpha = 1;
	}

	if (win) {
		imlib_context_set_display(win->env.dpy);
		imlib_context_set_visual(win->env.vis);
		imlib_context_set_colormap(win->env.cmap);
	}
}

#ifdef GIF_SUPPORT
/* Originally based on, but in its current form merely inspired by Imlib2's
 * src/modules/loaders/loader_gif.c:load(), written by Carsten Haitzler.
 */
int img_load_gif(img_t *img, const fileinfo_t *file) {
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
	int err = 0, transp = -1;
	unsigned int delay = 0;

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
		return 0;
	}
	bg = gif->SBackGroundColor;
	sw = gif->SWidth;
	sh = gif->SHeight;

	do {
		if (DGifGetRecordType(gif, &rec) == GIF_ERROR) {
			err = 1;
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
				err = 1;
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
				err = 1;
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
		img->multi.animate = 0;
	}

	imlib_context_set_image(img->im);

	return !err;
}
#endif /* GIF_SUPPORT */

int img_load(img_t *img, const fileinfo_t *file) {
	const char *fmt;

	if (!img || !file || !file->name || !file->path)
		return 0;

	if (access(file->path, R_OK) || !(img->im = imlib_load_image(file->path))) {
		warn("could not open image: %s", file->name);
		return 0;
	}

	imlib_context_set_image(img->im);
	imlib_image_set_changes_on_disk();
	imlib_context_set_anti_alias(img->aa);

	fmt = imlib_image_format();
#ifdef GIF_SUPPORT
	if (!strcmp(fmt, "gif"))
		img_load_gif(img, file);
#else
	/* avoid unused-but-set-variable warning */
	(void) fmt;
#endif

	img->scalemode = options->scalemode;
	img->re = 0;
	img->checkpan = 0;

	img->w = imlib_image_get_width();
	img->h = imlib_image_get_height();

	return 1;
}

void img_close(img_t *img, int decache) {
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

void img_check_pan(img_t *img, win_t *win) {
	if (!img || !win)
		return;

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
}

int img_fit(img_t *img, win_t *win) {
	float oz, zw, zh;

	if (!img || !win)
		return 0;

	oz = img->zoom;
	zw = (float) win->w / (float) img->w;
	zh = (float) win->h / (float) img->h;

	img->zoom = MIN(zw, zh);
	img->zoom = MAX(img->zoom, zoom_min);
	img->zoom = MIN(img->zoom, zoom_max);

	return oz != img->zoom;
}

void img_render(img_t *img, win_t *win) {
	int sx, sy, sw, sh;
	int dx, dy, dw, dh;

	if (!img || !img->im || !win)
		return;

	if (img->scalemode != SCALE_ZOOM) {
		img_fit(img, win);
		if (img->scalemode == SCALE_DOWN && img->zoom > 1.0)
			img->zoom = 1.0;
	}

	if (!img->re) {
		/* rendered for the first time */
		img->re = 1;
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
		img_check_pan(img, win);
		img->checkpan = 0;
	}

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
}

int img_fit_win(img_t *img, win_t *win) {
	if (!img || !img->im || !win)
		return 0;

	img->scalemode = SCALE_FIT;
	return img_fit(img, win);
}

int img_center(img_t *img, win_t *win) {
	int ox, oy;

	if (!img || !win)
		return 0;
	
	ox = img->x;
	oy = img->y;

	img->x = (win->w - img->w * img->zoom) / 2;
	img->y = (win->h - img->h * img->zoom) / 2;
	
	return ox != img->x || oy != img->y;
}

int img_zoom(img_t *img, win_t *win, float z) {
	if (!img || !img->im || !win)
		return 0;

	z = MAX(z, zoom_min);
	z = MIN(z, zoom_max);

	img->scalemode = SCALE_ZOOM;

	if (z != img->zoom) {
		img->x = win->w / 2 - (win->w / 2 - img->x) * z / img->zoom;
		img->y = win->h / 2 - (win->h / 2 - img->y) * z / img->zoom;
		img->zoom = z;
		img->checkpan = 1;
		return 1;
	} else {
		return 0;
	}
}

int img_zoom_in(img_t *img, win_t *win) {
	int i;

	if (!img || !img->im || !win)
		return 0;

	for (i = 1; i < ARRLEN(zoom_levels); i++) {
		if (zoom_levels[i] > img->zoom * 100.0)
			return img_zoom(img, win, zoom_levels[i] / 100.0);
	}
	return 0;
}

int img_zoom_out(img_t *img, win_t *win) {
	int i;

	if (!img || !img->im || !win)
		return 0;

	for (i = ARRLEN(zoom_levels) - 2; i >= 0; i--) {
		if (zoom_levels[i] < img->zoom * 100.0)
			return img_zoom(img, win, zoom_levels[i] / 100.0);
	}
	return 0;
}

int img_move(img_t *img, win_t *win, int dx, int dy) {
	int ox, oy;

	if (!img || !img->im || !win)
		return 0;

	ox = img->x;
	oy = img->y;

	img->x += dx;
	img->y += dy;

	img_check_pan(img, win);

	return ox != img->x || oy != img->y;
}

int img_pan(img_t *img, win_t *win, direction_t dir, int screen) {
	if (!img || !img->im || !win)
		return 0;

	switch (dir) {
		case DIR_LEFT:
			return img_move(img, win, win->w / (screen ? 1 : 5), 0);
		case DIR_RIGHT:
			return img_move(img, win, win->w / (screen ? 1 : 5) * -1, 0);
		case DIR_UP:
			return img_move(img, win, 0, win->h / (screen ? 1 : 5));
		case DIR_DOWN:
			return img_move(img, win, 0, win->h / (screen ? 1 : 5) * -1);
	}

	return 0;
}

int img_pan_edge(img_t *img, win_t *win, direction_t dir) {
	int ox, oy;

	if (!img || !img->im || !win)
		return 0;

	ox = img->x;
	oy = img->y;

	switch (dir) {
		case DIR_LEFT:
			img->x = 0;
			break;
		case DIR_RIGHT:
			img->x = win->w - img->w * img->zoom;
			break;
		case DIR_UP:
			img->y = 0;
			break;
		case DIR_DOWN:
			img->y = win->h - img->h * img->zoom;
			break;
	}

	img_check_pan(img, win);

	return ox != img->x || oy != img->y;
}

void img_rotate(img_t *img, win_t *win, int d) {
	int ox, oy, tmp;

	if (!img || !img->im || !win)
		return;

	ox = d == 1 ? img->x : win->w - img->x - img->w * img->zoom;
	oy = d == 3 ? img->y : win->h - img->y - img->h * img->zoom;

	imlib_context_set_image(img->im);
	imlib_image_orientate(d);

	img->x = oy + (win->w - win->h) / 2;
	img->y = ox + (win->h - win->w) / 2;

	tmp = img->w;
	img->w = img->h;
	img->h = tmp;

	img->checkpan = 1;
}

void img_rotate_left(img_t *img, win_t *win) {
	img_rotate(img, win, 3);
}

void img_rotate_right(img_t *img, win_t *win) {
	img_rotate(img, win, 1);
}

void img_toggle_antialias(img_t *img) {
	if (img && img->im) {
		img->aa ^= 1;
		imlib_context_set_image(img->im);
		imlib_context_set_anti_alias(img->aa);
	}
}

int img_frame_goto(img_t *img, int n) {
	if (!img || n < 0 || n >= img->multi.cnt)
		return 0;

	if (n == img->multi.sel)
		return 0;

	img->multi.sel = n;
	img->im = img->multi.frames[n].im;

	imlib_context_set_image(img->im);
	img->w = imlib_image_get_width();
	img->h = imlib_image_get_height();
	img->checkpan = 1;

	return 1;
}

int img_frame_navigate(img_t *img, int d) {
	if (!img || !img->multi.cnt || !d)
		return 0;

	d += img->multi.sel;
	if (d < 0)
		d = 0;
	else if (d >= img->multi.cnt)
		d = img->multi.cnt - 1;

	return img_frame_goto(img, d);
}

int img_frame_animate(img_t *img, int restart) {
	if (!img || !img->multi.cnt)
		return 0;

	if (img->multi.sel + 1 >= img->multi.cnt) {
		if (restart || GIF_LOOP) {
			img_frame_goto(img, 0);
		} else {
			img->multi.animate = 0;
			return 0;
		}
	} else if (!restart) {
		img_frame_goto(img, img->multi.sel + 1);
	}

	img->multi.animate = 1;

	return img->multi.frames[img->multi.sel].delay;
}
