/* sxiv: window.h
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

#ifndef WINDOW_H
#define WINDOW_H

#include <X11/Xlib.h>

typedef struct win_env_s {
	Display *dpy;
	int scr;
	int scrw, scrh;
	Visual *vis;
	Colormap cmap;
	int depth;
} win_env_t;

typedef struct win_s {
	Window xwin;
	win_env_t env;

	unsigned long bgcol;
	Pixmap pm;

	int w;
	int h;
	int x;
	int y;

	int bw;
	int fullscreen;
} win_t;

void win_open(win_t*);
void win_close(win_t*);

void win_set_title(win_t*, const char*);

int win_configure(win_t*, XConfigureEvent*);
void win_toggle_fullscreen(win_t*);

void win_clear(win_t*);
void win_draw(win_t*);

#endif /* WINDOW_H */
