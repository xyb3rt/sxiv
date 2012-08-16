/* sxiv: commands.h
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

bool it_quit(arg_t);
bool it_switch_mode(arg_t);
bool it_toggle_fullscreen(arg_t);
bool it_toggle_bar(arg_t);
bool t_reload_all(arg_t);
bool it_reload_image(arg_t);
bool it_remove_image(arg_t);
bool i_navigate(arg_t);
bool i_alternate(arg_t);
bool it_first(arg_t);
bool it_n_or_last(arg_t);
bool i_navigate_frame(arg_t);
bool i_toggle_animation(arg_t);
bool it_scroll_move(arg_t);
bool it_scroll_screen(arg_t);
bool i_scroll_to_edge(arg_t);
bool i_drag(arg_t);
bool i_zoom(arg_t);
bool i_set_zoom(arg_t);
bool i_fit_to_win(arg_t);
bool i_fit_to_img(arg_t);
bool i_rotate(arg_t);
bool i_flip(arg_t);
bool i_toggle_slideshow(arg_t);
bool i_adjust_slideshow(arg_t);
bool i_reset_slideshow(arg_t);
bool i_toggle_antialias(arg_t);
bool it_toggle_alpha(arg_t);
bool it_open_with(arg_t);
bool it_shell_cmd(arg_t);

#endif /* COMMANDS_H */
