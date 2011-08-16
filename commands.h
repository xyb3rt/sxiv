/* sxiv: commands.h
 * Copyright (c) 2011 Bert Muennich <muennich at informatik.hu-berlin.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, 5th Floor, Boston, MA 02110-1301, USA.
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include <X11/Xlib.h>

typedef void* arg_t;
typedef int (*command_f)(arg_t);

typedef struct {
	Bool ctrl;
	KeySym ksym;
	command_f cmd;
	arg_t arg;
} keymap_t;

typedef struct {
	Bool ctrl;
	Bool shift;
	unsigned int button;
	command_f cmd;
	arg_t arg;
} button_t;

int it_quit(arg_t);
int it_switch_mode(arg_t);
int it_toggle_fullscreen(arg_t);
int it_reload_image(arg_t);
int it_remove_image(arg_t);
int i_navigate(arg_t);
int it_first(arg_t);
int it_last(arg_t);
int i_navigate_frame(arg_t);
int it_move(arg_t);
int i_pan_screen(arg_t);
int i_pan_edge(arg_t);
int i_drag(arg_t);
int i_zoom(arg_t);
int i_fit_to_win(arg_t);
int i_fit_to_img(arg_t);
int i_rotate(arg_t);
int i_toggle_antialias(arg_t);
int i_toggle_alpha(arg_t);
int it_open_with(arg_t);
int it_shell_cmd(arg_t);

#endif /* COMMANDS_H */
