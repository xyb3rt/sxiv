/* sxiv: window.h
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

#ifndef WINDOW_H
#define WINDOW_H

#include <X11/Xlib.h>

#include "types.h"

typedef struct {
	Display *dpy;
	int scr;
	int scrw, scrh;
	Visual *vis;
	Colormap cmap;
	int depth;
} win_env_t;

typedef struct {
	Window xwin;
	win_env_t env;

	unsigned long black;
	unsigned long white;
	unsigned long bgcol;
	unsigned long selcol;
	Pixmap pm;

	int x;
	int y;
	unsigned int w;
	unsigned int h;
	unsigned int bw;

	bool fullscreen;
} win_t;

extern Atom wm_delete_win;

void win_init(win_t*);
void win_open(win_t*);
void win_close(win_t*);

bool win_configure(win_t*, XConfigureEvent*);
bool win_moveresize(win_t*, int, int, unsigned int, unsigned int);

void win_toggle_fullscreen(win_t*);

void win_clear(win_t*);
void win_draw(win_t*);
void win_draw_rect(win_t*, Pixmap, int, int, int, int, bool, int,
                   unsigned long);

void win_set_title(win_t*, const char*);
void win_set_cursor(win_t*, cursor_t);

#endif /* WINDOW_H */
