/* sxiv: events.c
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

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "events.h"
#include "window.h"

extern Display *dpy;

void on_expose(app_t *app, XEvent *ev) {
}

void on_configurenotify(app_t *app, XEvent *ev) {
	if (app == NULL || ev == NULL)
		return;
	
	win_configure(&app->win, &ev->xconfigure);
}

void on_keypress(app_t *app, XEvent *ev) {
	KeySym keysym;
	XKeyEvent *kev;

	if (app == NULL || ev == NULL)
		return;
	
	kev = &ev->xkey;
	keysym = XKeycodeToKeysym(dpy, (KeyCode) kev->keycode, 0);

	switch (keysym) {
		case XK_q:
			app_quit(app);
			exit(0);
	}
}

static void (*handler[LASTEvent])(app_t*, XEvent*) = {
	[Expose] = on_expose,
	[ConfigureNotify] = on_configurenotify,
	[KeyPress] = on_keypress
};

void event_loop(app_t *app) {
	XEvent ev;

	while (!XNextEvent(dpy, &ev)) {
		if (handler[ev.type])
			handler[ev.type](app, &ev);
	}
}
