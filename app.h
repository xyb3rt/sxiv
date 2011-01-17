/* sxiv: app.h
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

#ifndef APP_H
#define APP_H

#include "image.h"
#include "window.h"

typedef struct app_s {
	char **filenames;
	unsigned int filecnt;
	unsigned int fileidx;
	img_t img;
	win_t win;
} app_t;

void app_init(app_t*);
void app_run(app_t*);
void app_quit(app_t*);

#endif /* APP_H */
