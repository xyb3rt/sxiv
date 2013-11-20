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

#ifndef COMMANDS_H
#define COMMANDS_H

#include <X11/Xlib.h>

#include "types.h"

typedef void* arg_t;
typedef bool (*command_f)(arg_t);

typedef struct {
	bool ctrl;
	KeySym ksym;
	command_f cmd;
	arg_t arg;
} keymap_t;

typedef struct {
	bool ctrl;
	bool shift;
	unsigned int button;
	command_f cmd;
	arg_t arg;
} button_t;

bool it_quit();
bool it_switch_mode();
bool it_toggle_fullscreen();
bool it_toggle_bar();
bool t_reload_all();
bool it_reload_image();
bool it_remove_image();
bool i_navigate(long);
bool i_alternate();
bool it_first();
bool it_n_or_last(int);
bool i_navigate_frame(long);
bool i_toggle_animation();
bool it_scroll_move(direction_t, int);
bool it_scroll_screen(direction_t);
bool i_scroll_to_edge(direction_t);
bool i_drag(arg_t);
bool i_zoom(long);
bool i_set_zoom(long);
bool i_fit_to_win(scalemode_t);
bool i_fit_to_img();
bool i_rotate(degree_t);
bool i_flip(flipdir_t);
bool i_toggle_antialias();
bool it_toggle_alpha();
bool it_open_with(arg_t);
bool it_shell_cmd(arg_t);

#endif /* COMMANDS_H */
