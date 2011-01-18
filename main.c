/* sxiv: main.c
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

#include <stdlib.h>

#include "sxiv.h"
#include "app.h"

app_t app;

int main(int argc, char **argv) {

	// TODO: parse cmd line arguments properly
	app.filenames = argv + 1;
	app.filecnt = argc - 1;

	app_init(&app);
	app_run(&app);
	app_quit(&app);

	return 0;
}

void cleanup() {
	static int in = 0;

	if (!in++)
		app_quit(&app);
}
