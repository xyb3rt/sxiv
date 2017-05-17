/* Copyright 2017 Max Voit
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

#ifndef AUTORELOAD_H
#define AUTORELOAD_H

#include "types.h"

typedef struct {
	int fd;
	int wd_dir;
	int wd_file;
	char *filename;
} arl_t;

void arl_init(arl_t*);
void arl_cleanup(arl_t*);
void arl_setup(arl_t*, const char* /* result of realpath(3) */);
bool arl_handle(arl_t*);

#endif /* AUTORELOAD_H */
