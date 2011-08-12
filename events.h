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

typedef void* arg_t;

typedef struct {
	Bool ctrl;
	KeySym ksym;
	int (*handler)(arg_t);
	arg_t arg;
} keymap_t;

typedef struct {
	Bool ctrl;
	Bool shift;
	unsigned int button;
	int (*handler)(arg_t);
	arg_t arg;
} button_t;

void run();

/* handler functions for key and button mappings: */
int quit(arg_t);
int reload(arg_t);
int toggle_fullscreen(arg_t);
int toggle_antialias(arg_t);
int toggle_alpha(arg_t);
int switch_mode(arg_t);
int navigate(arg_t);
int first(arg_t);
int last(arg_t);
int remove_image(arg_t);
int move(arg_t);
int pan_screen(arg_t);
int pan_edge(arg_t);
int drag(arg_t);
int rotate(arg_t);
int zoom(arg_t);
int fit_to_win(arg_t);
int fit_to_img(arg_t);
int open_with(arg_t);
int run_command(arg_t);

#endif /* EVENTS_H */
