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

typedef enum {
	CMD_INVALID = -1,
	CMD_OK      =  0,
	CMD_DIRTY   =  1
} cmdreturn_t;

typedef void* arg_t;
typedef cmdreturn_t (*command_f)(arg_t);

typedef struct {
	unsigned int mask;
	KeySym ksym;
	command_f cmd;
	arg_t arg;
} keymap_t;

typedef struct {
	unsigned int mask;
	unsigned int button;
	command_f cmd;
	arg_t arg;
} button_t;

cmdreturn_t it_quit(arg_t);
cmdreturn_t it_switch_mode(arg_t);
cmdreturn_t it_toggle_fullscreen(arg_t);
cmdreturn_t it_toggle_bar(arg_t);
cmdreturn_t it_prefix_external(arg_t);
cmdreturn_t t_reload_all(arg_t);
cmdreturn_t it_reload_image(arg_t);
cmdreturn_t it_remove_image(arg_t);
cmdreturn_t i_navigate(arg_t);
cmdreturn_t i_alternate(arg_t);
cmdreturn_t it_first(arg_t);
cmdreturn_t it_n_or_last(arg_t);
cmdreturn_t i_navigate_frame(arg_t);
cmdreturn_t i_toggle_animation(arg_t);
cmdreturn_t it_toggle_image_mark(arg_t);
cmdreturn_t it_reverse_marks(arg_t);
cmdreturn_t it_navigate_marked(arg_t);
cmdreturn_t it_scroll_move(arg_t);
cmdreturn_t it_scroll_screen(arg_t);
cmdreturn_t i_scroll_to_edge(arg_t);
cmdreturn_t i_drag(arg_t);
cmdreturn_t i_zoom(arg_t);
cmdreturn_t i_set_zoom(arg_t);
cmdreturn_t i_fit_to_win(arg_t);
cmdreturn_t i_rotate(arg_t);
cmdreturn_t i_flip(arg_t);
cmdreturn_t i_slideshow(arg_t);
cmdreturn_t i_toggle_antialias(arg_t);
cmdreturn_t i_toggle_alpha(arg_t);
cmdreturn_t i_change_gamma(arg_t);

#endif /* COMMANDS_H */
