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

void arl_cleanup(void);
void arl_handle(void);
void arl_init(void);
void arl_setup(void);
void arl_setup_dir(void);

typedef struct {
	int fd;
	int wd;
	bool watching_dir;
} autoreload_t;

extern autoreload_t autoreload;
extern int fileidx;
extern fileinfo_t *files;

void load_image(int);
void redraw(void);

#endif /* AUTORELOAD_H */
