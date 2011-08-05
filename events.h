/* sxiv: events.h
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

#ifndef EVENTS_H
#define EVENTS_H

#include <X11/Xlib.h>

typedef struct {
	KeySym ksym;
	Bool reload;
	const char *cmdline;
} command_t;

typedef int arg_t;

typedef struct {
	KeySym ksym;
	int (*handler)(XEvent*, arg_t);
	arg_t arg;
} keymap_t;

typedef struct {
	unsigned int mod;
	unsigned int button;
	int (*handler)(XEvent*, arg_t);
	arg_t arg;
} button_t;

void run();

/* handler functions for key and button mappings: */
int quit(XEvent*, arg_t);
int reload(XEvent*, arg_t);
int toggle_fullscreen(XEvent*, arg_t);
int toggle_antialias(XEvent*, arg_t);
int toggle_alpha(XEvent*, arg_t);
int switch_mode(XEvent*, arg_t);
int navigate(XEvent*, arg_t);
int first(XEvent*, arg_t);
int last(XEvent*, arg_t);
int remove_image(XEvent*, arg_t);
int move(XEvent*, arg_t);
int pan_screen(XEvent*, arg_t);
int pan_edge(XEvent*, arg_t);
int drag(XEvent*, arg_t);
int rotate(XEvent*, arg_t);
int zoom(XEvent*, arg_t);
int fit_to_win(XEvent*, arg_t);
int fit_to_img(XEvent*, arg_t);

#endif /* EVENTS_H */
