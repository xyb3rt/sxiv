/* Copyright 2011, 2014 Bert Muennich
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

typedef int arg_t;
typedef bool (*cmd_f)(arg_t);

#define G_CMD(c) g_##c,
#define I_CMD(c) i_##c,
#define T_CMD(c) t_##c,

typedef enum {
#include "commands.lst"
	CMD_COUNT
} cmd_id_t;

typedef struct {
	int mode;
	cmd_f func;
} cmd_t;

typedef struct {
	unsigned int mask;
	KeySym ksym;
	cmd_id_t cmd;
	arg_t arg;
} keymap_t;

typedef struct {
	unsigned int mask;
	unsigned int button;
	cmd_id_t cmd;
	arg_t arg;
} button_t;

const extern cmd_t cmds[CMD_COUNT];

#endif /* COMMANDS_H */
