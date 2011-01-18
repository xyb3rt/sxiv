/* sxiv: image.c
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
#include <stdio.h>

#include "sxiv.h"
#include "image.h"

void imlib_init(win_t *win) {
	if (!win)
		return;
	
	imlib_context_set_display(win->env.dpy);
	imlib_context_set_visual(win->env.vis);
	imlib_context_set_colormap(win->env.cmap);
	imlib_context_set_drawable(win->xwin);
}

void img_load(img_t *img, char *filename) {
	if (!img || !filename)
		return;

	if (imlib_context_get_image())
		imlib_free_image();

	if (!(img->im = imlib_load_image(filename)))
		DIE("could not open image: %s", filename);
	
	imlib_context_set_image(img->im);
	
	img->w = imlib_image_get_width();
	img->h = imlib_image_get_height();
}
