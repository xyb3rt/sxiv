/* sxiv: app.c
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

#include <X11/Xlib.h>

#include "sxiv.h"
#include "app.h"
#include "events.h"

void app_init(app_t *app) {
	if (!app)
		return;

	app->fileidx = 0;

	app->img.zoom = 100;
	app->img.scalemode = SCALE_MODE;

	app->win.w = WIN_WIDTH;
	app->win.h = WIN_HEIGHT;

	win_open(&app->win);
	
	imlib_init(&app->win);
}

void app_run(app_t *app) {
	app_load_image(app);
	event_loop(app);
}

void app_quit(app_t *app) {
}

void app_load_image(app_t *app) {
	if (!app || app->fileidx >= app->filecnt || !app->filenames)
		return;

	img_load(&app->img, app->filenames[app->fileidx]);
}
