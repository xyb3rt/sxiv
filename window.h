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

#ifndef WINDOW_H
#define WINDOW_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#include "types.h"

enum {
	BAR_L_LEN = 512,
	BAR_R_LEN = 64
};

enum {
	ATOM_WM_DELETE_WINDOW,
	ATOM__NET_WM_NAME,
	ATOM__NET_WM_ICON_NAME,
	ATOM__NET_WM_ICON,
	ATOM__NET_WM_STATE,
	ATOM__NET_WM_STATE_FULLSCREEN,
	ATOM__NET_SUPPORTED,
	ATOM_COUNT
};

typedef struct {
	Display *dpy;
	int scr;
	int scrw, scrh;
	Visual *vis;
	Colormap cmap;
	int depth;
} win_env_t;

typedef struct {
	size_t size;
	char *p;
	char *buf;
} win_bar_t;

typedef struct {
	Window xwin;
	win_env_t env;

	XftColor bgcol;
	XftColor fscol;
	XftColor selcol;

	int x;
	int y;
	unsigned int w;
	unsigned int h; /* = win height - bar height */
	unsigned int bw;

	bool fullscreen;

	struct {
		int w;
		int h;
		Pixmap pm;
	} buf;

	struct {
		unsigned int h;
		win_bar_t l;
		win_bar_t r;
		XftColor bgcol;
		XftColor fgcol;
	} bar;
} win_t;

extern Atom atoms[ATOM_COUNT];

void win_init(win_t*);
void win_open(win_t*);
CLEANUP void win_close(win_t*);

bool win_configure(win_t*, XConfigureEvent*);

void win_toggle_fullscreen(win_t*);
void win_toggle_bar(win_t*);

void win_clear(win_t*);
void win_draw(win_t*);
void win_draw_rect(win_t*, int, int, int, int, bool, int, unsigned long);

int win_textwidth(const win_env_t*, const char*, unsigned int, bool);

void win_set_title(win_t*, const char*);
void win_set_cursor(win_t*, cursor_t);
void win_cursor_pos(win_t*, int*, int*);

#endif /* WINDOW_H */
