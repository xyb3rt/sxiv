/* Copyright 2011-2013 Bert Muennich
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

#define _POSIX_C_SOURCE 200112L
#define _MAPPINGS_CONFIG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>

#include "types.h"
#include "commands.h"
#include "image.h"
#include "options.h"
#include "thumbs.h"
#include "util.h"
#include "window.h"
#include "config.h"

enum {
	FILENAME_CNT = 1024,
	TITLE_LEN    = 256
};

typedef struct {
	const char *name;
	char *cmd;
} exec_t;

typedef struct {
	struct timeval when;
	bool active;
	timeout_f handler;
} timeout_t;

/* timeout handler functions: */
void redraw(void);
void reset_cursor(void);
void animate(void);
void slideshow(void);
void clear_resize(void);

appmode_t mode;
img_t img;
tns_t tns;
win_t win;

fileinfo_t *files;
int filecnt, fileidx;
int alternate;

int prefix;
bool extprefix;

bool resized = false;

struct {
  char *cmd;
  int fd;
  unsigned int i, lastsep;
  bool open;
} info;

struct {
	char *cmd;
	bool warned;
} keyhandler;

timeout_t timeouts[] = {
	{ { 0, 0 }, false, redraw       },
	{ { 0, 0 }, false, reset_cursor },
	{ { 0, 0 }, false, animate      },
	{ { 0, 0 }, false, slideshow    },
	{ { 0, 0 }, false, clear_resize },
};

void cleanup(void)
{
	static bool in = false;

	if (!in) {
		in = true;
		img_close(&img, false);
		tns_free(&tns);
		win_close(&win);
	}
}

void check_add_file(char *filename)
{
	const char *bn;

	if (filename == NULL || *filename == '\0')
		return;

	if (access(filename, R_OK) < 0) {
		warn("could not open file: %s", filename);
		return;
	}

	if (fileidx == filecnt) {
		filecnt *= 2;
		files = (fileinfo_t*) s_realloc(files, filecnt * sizeof(fileinfo_t));
	}

#if defined _BSD_SOURCE || defined _XOPEN_SOURCE && \
    ((_XOPEN_SOURCE - 0) >= 500 || defined _XOPEN_SOURCE_EXTENDED)

	if ((files[fileidx].path = realpath(filename, NULL)) == NULL) {
		warn("could not get real path of file: %s\n", filename);
		return;
	}
#else
	if (*filename != '/') {
		if ((files[fileidx].path = absolute_path(filename)) == NULL) {
			warn("could not get absolute path of file: %s\n", filename);
			return;
		}
	} else {
		files[fileidx].path = NULL;
	}
#endif

	files[fileidx].loaded = false;
	files[fileidx].name = s_strdup(filename);
	if (files[fileidx].path == NULL)
		files[fileidx].path = files[fileidx].name;
	if ((bn = strrchr(files[fileidx].name , '/')) != NULL && bn[1] != '\0')
		files[fileidx].base = ++bn;
	else
		files[fileidx].base = files[fileidx].name;
	fileidx++;
}

void remove_file(int n, bool manual)
{
	if (n < 0 || n >= filecnt)
		return;

	if (filecnt == 1) {
		if (!manual)
			fprintf(stderr, "sxiv: no more files to display, aborting\n");
		cleanup();
		exit(manual ? EXIT_SUCCESS : EXIT_FAILURE);
	}

	if (files[n].path != files[n].name)
		free((void*) files[n].path);
	free((void*) files[n].name);

	if (n + 1 < filecnt)
		memmove(files + n, files + n + 1, (filecnt - n - 1) * sizeof(fileinfo_t));
	if (n + 1 < tns.cnt) {
		memmove(tns.thumbs + n, tns.thumbs + n + 1, (tns.cnt - n - 1) *
		        sizeof(thumb_t));
		memset(tns.thumbs + tns.cnt - 1, 0, sizeof(thumb_t));
	}

	filecnt--;
	if (n < tns.cnt)
		tns.cnt--;
	if (n < alternate)
		alternate--;
}

void set_timeout(timeout_f handler, int time, bool overwrite)
{
	int i;

	for (i = 0; i < ARRLEN(timeouts); i++) {
		if (timeouts[i].handler == handler) {
			if (!timeouts[i].active || overwrite) {
				gettimeofday(&timeouts[i].when, 0);
				TV_ADD_MSEC(&timeouts[i].when, time);
				timeouts[i].active = true;
			}
			return;
		}
	}
}

void reset_timeout(timeout_f handler)
{
	int i;

	for (i = 0; i < ARRLEN(timeouts); i++) {
		if (timeouts[i].handler == handler) {
			timeouts[i].active = false;
			return;
		}
	}
}

bool check_timeouts(struct timeval *t)
{
	int i = 0, tdiff, tmin = -1;
	struct timeval now;

	while (i < ARRLEN(timeouts)) {
		if (timeouts[i].active) {
			gettimeofday(&now, 0);
			tdiff = TV_DIFF(&timeouts[i].when, &now);
			if (tdiff <= 0) {
				timeouts[i].active = false;
				if (timeouts[i].handler != NULL)
					timeouts[i].handler();
				i = tmin = -1;
			} else if (tmin < 0 || tdiff < tmin) {
				tmin = tdiff;
			}
		}
		i++;
	}
	if (tmin > 0 && t != NULL)
		TV_SET_MSEC(t, tmin);
	return tmin > 0;
}

void open_info(void)
{
	static pid_t pid;
	int pfd[2];
    char buffer[256];
    char* infocmd[7];

	if (info.cmd == NULL || info.open || win.bar.h == 0)
		return;
	if (info.fd != -1) {
		close(info.fd);
		kill(pid, SIGTERM);
		info.fd = -1;
	}
	win.bar.l[0] = '\0';

	if (pipe(pfd) < 0)
		return;
	pid = fork();
	if (pid > 0) {
		close(pfd[1]);
		fcntl(pfd[0], F_SETFL, O_NONBLOCK);
		info.fd = pfd[0];
		info.i = info.lastsep = 0;
		info.open = true;
	} else if (pid == 0) {
		close(pfd[0]);
		dup2(pfd[1], 1);

        infocmd[0] = info.cmd;
        infocmd[1] = s_strdup(files[fileidx].name);
        snprintf(buffer, 256, "%i", img.w);       infocmd[2] = s_strdup(buffer);
        snprintf(buffer, 256, "%i", img.h);       infocmd[3] = s_strdup(buffer);
        snprintf(buffer, 256, "%i", fileidx + 1); infocmd[4] = s_strdup(buffer);
        snprintf(buffer, 256, "%i", filecnt);     infocmd[5] = s_strdup(buffer);
        infocmd[6] = NULL;
		execv(info.cmd, infocmd);
		warn("could not exec: %s", info.cmd);
		exit(EXIT_FAILURE);
	}
}

void read_info(void)
{
	ssize_t i, n;
	char buf[BAR_L_LEN];

	while (true) {
		n = read(info.fd, buf, sizeof(buf));
		if (n < 0 && errno == EAGAIN)
			return;
		else if (n == 0)
			goto end;
		for (i = 0; i < n; i++) {
			if (buf[i] == '\n') {
				if (info.lastsep == 0) {
					win.bar.l[info.i++] = ' ';
					info.lastsep = 1;
				}
			} else {
				win.bar.l[info.i++] = buf[i];
				info.lastsep = 0;
			}
			if (info.i + 1 == sizeof(win.bar.l))
				goto end;
		}
	}
end:
	info.i -= info.lastsep;
	win.bar.l[info.i] = '\0';
	win_draw(&win);
	close(info.fd);
	info.fd = -1;
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

void load_image(int new)
{
	if (new < 0 || new >= filecnt)
		return;

	win_set_cursor(&win, CURSOR_WATCH);
	reset_timeout(slideshow);

	if (new != fileidx)
		alternate = fileidx;

	img_close(&img, false);
	while (!img_load(&img, &files[new])) {
		remove_file(new, false);
		if (new >= filecnt)
			new = filecnt - 1;
		else if (new > 0 && new < fileidx)
			new--;
	}
	files[new].loaded = true;
	fileidx = new;

	info.open = false;
	open_info();

	if (img.multi.cnt > 0 && img.multi.animate)
		set_timeout(animate, img.multi.frames[img.multi.sel].delay, true);
	else
		reset_timeout(animate);
}

void update_info(void)
{
	int sel;
	unsigned int i, fn, fw, n;
	unsigned int llen = sizeof(win.bar.l), rlen = sizeof(win.bar.r);
	char *lt = win.bar.l, *rt = win.bar.r, title[TITLE_LEN];
	const char * mark;
	bool ow_info;

	for (fw = 0, i = filecnt; i > 0; fw++, i /= 10);
	sel = mode == MODE_IMAGE ? fileidx : tns.sel;

	/* update window title */
	if (mode == MODE_THUMB) {
		win_set_title(&win, "sxiv");
	} else {
		snprintf(title, sizeof(title), "sxiv - %s", files[sel].name);
		win_set_title(&win, title);
	}

	/* update bar contents */
	if (win.bar.h == 0)
		return;
	mark = files[sel].marked ? "* " : "";
	if (mode == MODE_THUMB) {
		if (tns.cnt == filecnt) {
			n = snprintf(rt, rlen, "%s%0*d/%d", mark, fw, sel + 1, filecnt);
			ow_info = true;
		} else {
			snprintf(lt, llen, "Loading... %0*d/%d", fw, tns.cnt, filecnt);
			rt[0] = '\0';
			ow_info = false;
		}
	} else {
		n = snprintf(rt, rlen, "%s", mark);
		if (img.ss.on)
			n += snprintf(rt + n, rlen - n, "%ds | ", img.ss.delay);
		if (img.gamma != 0)
			n += snprintf(rt + n, rlen - n, "G%+d | ", img.gamma);
		n += snprintf(rt + n, rlen - n, "%3d%% | ", (int) (img.zoom * 100.0));
		if (img.multi.cnt > 0) {
			for (fn = 0, i = img.multi.cnt; i > 0; fn++, i /= 10);
			n += snprintf(rt + n, rlen - n, "%0*d/%d | ",
			              fn, img.multi.sel + 1, img.multi.cnt);
		}
		n += snprintf(rt + n, rlen - n, "%0*d/%d", fw, sel + 1, filecnt);
		ow_info = info.cmd == NULL;
	}
	if (ow_info) {
		fn = strlen(files[sel].name);
		if (fn < llen &&
		    win_textwidth(files[sel].name, fn, true) +
		    win_textwidth(rt, n, true) < win.w)
		{
			strncpy(lt, files[sel].name, llen);
		} else {
			strncpy(lt, files[sel].base, llen);
		}
	}
}

void redraw(void)
{
	int t;

	if (mode == MODE_IMAGE) {
		img_render(&img);
		if (img.ss.on) {
			t = img.ss.delay * 1000;
			if (img.multi.cnt > 0 && img.multi.animate)
				t = MAX(t, img.multi.length);
			set_timeout(slideshow, t, false);
		}
	} else {
		tns_render(&tns);
	}
	update_info();
	win_draw(&win);
	reset_timeout(redraw);
	reset_cursor();
}

void reset_cursor(void)
{
	int i;
	cursor_t cursor = CURSOR_NONE;

	if (mode == MODE_IMAGE) {
		for (i = 0; i < ARRLEN(timeouts); i++) {
			if (timeouts[i].handler == reset_cursor) {
				if (timeouts[i].active)
					cursor = CURSOR_ARROW;
				break;
			}
		}
	} else {
		if (tns.cnt != filecnt)
			cursor = CURSOR_WATCH;
		else
			cursor = CURSOR_ARROW;
	}
	win_set_cursor(&win, cursor);
}

void animate(void)
{
	if (img_frame_animate(&img, false)) {
		redraw();
		set_timeout(animate, img.multi.frames[img.multi.sel].delay, true);
	}
}

void slideshow(void)
{
	load_image(fileidx + 1 < filecnt ? fileidx + 1 : 0);
	redraw();
}

void clear_resize(void)
{
	resized = false;
}

void run_key_handler(const char *key, unsigned int mask)
{
	pid_t pid;
	int retval, status, n = mode == MODE_IMAGE ? fileidx : tns.sel;
	char kstr[32], oldbar[sizeof(win.bar.l)];
	bool restore_bar = mode == MODE_IMAGE && info.cmd != NULL;
	struct stat oldst, newst;

	if (keyhandler.cmd == NULL) {
		if (!keyhandler.warned) {
			warn("key handler not installed");
			keyhandler.warned = true;
		}
		return;
	}
	if (key == NULL)
		return;

	snprintf(kstr, sizeof(kstr), "%s%s%s%s",
	         mask & ControlMask ? "C-" : "",
	         mask & Mod1Mask    ? "M-" : "",
	         mask & ShiftMask   ? "S-" : "", key);

	if (restore_bar)
		memcpy(oldbar, win.bar.l, sizeof(win.bar.l));
	strncpy(win.bar.l, "Running key handler...", sizeof(win.bar.l));
	win_draw(&win);
	win_set_cursor(&win, CURSOR_WATCH);
	stat(files[n].path, &oldst);

	if ((pid = fork()) == 0) {
		execl(keyhandler.cmd, keyhandler.cmd, kstr, files[n].path, NULL);
		warn("could not exec key handler");
		exit(EXIT_FAILURE);
	} else if (pid < 0) {
		warn("could not fork key handler");
		goto end;
	}
	waitpid(pid, &status, 0);
	retval = WEXITSTATUS(status);
	if (WIFEXITED(status) == 0 || retval != 0)
		warn("key handler exited with non-zero return value: %d", retval);

	if (stat(files[n].path, &newst) == 0 &&
	    memcmp(&oldst.st_mtime, &newst.st_mtime, sizeof(oldst.st_mtime)) == 0)
	{
		/* file has not changed */
		goto end;
	}
	restore_bar = false;
	strncpy(win.bar.l, "Reloading image...", sizeof(win.bar.l));
	win_draw(&win);

	if (mode == MODE_IMAGE) {
		img_close(&img, true);
		load_image(fileidx);
	}
	if (!tns_load(&tns, n, &files[n], true, mode == MODE_IMAGE) &&
	    mode == MODE_THUMB)
	{
		remove_file(tns.sel, false);
		tns.dirty = true;
		if (tns.sel >= tns.cnt)
			tns.sel = tns.cnt - 1;
	}
end:
	if (restore_bar)
		memcpy(win.bar.l, oldbar, sizeof(win.bar.l));
	reset_cursor();
	redraw();
}

#define MODMASK(mask) ((mask) & (ShiftMask|ControlMask|Mod1Mask))

void on_keypress(XKeyEvent *kev)
{
	int i;
	unsigned int sh;
	KeySym ksym, shksym;
	char key;
	bool dirty = false;

	if (kev == NULL)
		return;

	if (kev->state & ShiftMask) {
		kev->state &= ~ShiftMask;
		XLookupString(kev, &key, 1, &shksym, NULL);
		kev->state |= ShiftMask;
	}
	XLookupString(kev, &key, 1, &ksym, NULL);
	sh = (kev->state & ShiftMask) && ksym != shksym ? ShiftMask : 0;

	if (IsModifierKey(ksym))
		return;
	if (ksym == XK_Escape && MODMASK(kev->state) == 0) {
		extprefix = False;
	} else if (extprefix) {
		run_key_handler(XKeysymToString(ksym), kev->state & ~sh);
		extprefix = False;
	} else if (key >= '0' && key <= '9') {
		/* number prefix for commands */
		prefix = prefix * 10 + (int) (key - '0');
		return;
	} else for (i = 0; i < ARRLEN(keys); i++) {
		if (keys[i].ksym == ksym &&
		    MODMASK(keys[i].mask | sh) == MODMASK(kev->state) &&
		    keys[i].cmd >= 0 && keys[i].cmd < CMD_COUNT &&
		    (cmds[keys[i].cmd].mode < 0 || cmds[keys[i].cmd].mode == mode))
		{
			if (cmds[keys[i].cmd].func(keys[i].arg))
				dirty = true;
		}
	}
	if (dirty)
		redraw();
	prefix = 0;
}

void on_buttonpress(XButtonEvent *bev)
{
	int i, sel;
	bool dirty = false;
	static Time firstclick;

	if (bev == NULL)
		return;

	if (mode == MODE_IMAGE) {
		win_set_cursor(&win, CURSOR_ARROW);
		set_timeout(reset_cursor, TO_CURSOR_HIDE, true);

		for (i = 0; i < ARRLEN(buttons); i++) {
			if (buttons[i].button == bev->button &&
			    MODMASK(buttons[i].mask) == MODMASK(bev->state) &&
			    buttons[i].cmd >= 0 && buttons[i].cmd < CMD_COUNT &&
			    (cmds[buttons[i].cmd].mode < 0 || cmds[buttons[i].cmd].mode == mode))
			{
				if (cmds[buttons[i].cmd].func(buttons[i].arg))
					dirty = true;
			}
		}
		if (dirty)
			redraw();
	} else {
		/* thumbnail mode (hard-coded) */
		switch (bev->button) {
			case Button1:
				if ((sel = tns_translate(&tns, bev->x, bev->y)) >= 0) {
					if (sel != tns.sel) {
						tns_highlight(&tns, tns.sel, false);
						tns_highlight(&tns, sel, true);
						tns.sel = sel;
						firstclick = bev->time;
						redraw();
					} else if (bev->time - firstclick <= TO_DOUBLE_CLICK) {
						mode = MODE_IMAGE;
						set_timeout(reset_cursor, TO_CURSOR_HIDE, true);
						load_image(tns.sel);
						redraw();
					} else {
						firstclick = bev->time;
					}
				}
				break;
			case Button3:
				if ((sel = tns_translate(&tns, bev->x, bev->y)) >= 0) {
					files[sel].marked = !files[sel].marked;
					tns_mark(&tns, sel, files[sel].marked);
					redraw();
				}
				break;
			case Button4:
			case Button5:
				if (tns_scroll(&tns, bev->button == Button4 ? DIR_UP : DIR_DOWN,
				               (bev->state & ControlMask) != 0))
					redraw();
				break;
		}
	}
	prefix = 0;
}

void run(void)
{
	int xfd;
	fd_set fds;
	struct timeval timeout;
	bool discard, to_set;
	XEvent ev, nextev;

	set_timeout(redraw, 25, false);

	while (true) {
		while (mode == MODE_THUMB && tns.cnt < filecnt &&
		       XPending(win.env.dpy) == 0)
		{
			/* load thumbnails */
			set_timeout(redraw, TO_REDRAW_THUMBS, false);
			if (tns_load(&tns, tns.cnt, &files[tns.cnt], false, false)) {
				tns.cnt++;
			} else {
				remove_file(tns.cnt, false);
				if (tns.sel > 0 && tns.sel >= tns.cnt)
					tns.sel--;
			}
			if (tns.cnt == filecnt)
				redraw();
			else
				check_timeouts(NULL);
		}

		while (XPending(win.env.dpy) == 0
		       && ((to_set = check_timeouts(&timeout)) || info.fd != -1))
		{
			/* check for timeouts & input */
			xfd = ConnectionNumber(win.env.dpy);
			FD_ZERO(&fds);
			FD_SET(xfd, &fds);
			if (info.fd != -1) {
				FD_SET(info.fd, &fds);
				xfd = MAX(xfd, info.fd);
			}
			select(xfd + 1, &fds, 0, 0, to_set ? &timeout : NULL);
			if (info.fd != -1 && FD_ISSET(info.fd, &fds))
				read_info();
		}

		do {
			XNextEvent(win.env.dpy, &ev);
			discard = false;
			if (XEventsQueued(win.env.dpy, QueuedAlready) > 0) {
				XPeekEvent(win.env.dpy, &nextev);
				switch (ev.type) {
					case ConfigureNotify:
						discard = ev.type == nextev.type;
						break;
					case KeyPress:
						discard = (nextev.type == KeyPress || nextev.type == KeyRelease)
						          && ev.xkey.keycode == nextev.xkey.keycode;
						break;
				}
			}
		} while (discard);

		switch (ev.type) {
			/* handle events */
			case ButtonPress:
				on_buttonpress(&ev.xbutton);
				break;
			case ClientMessage:
				if ((Atom) ev.xclient.data.l[0] == atoms[ATOM_WM_DELETE_WINDOW])
					return;
				break;
			case ConfigureNotify:
				if (win_configure(&win, &ev.xconfigure)) {
					if (mode == MODE_IMAGE) {
						img.dirty = true;
						img.checkpan = true;
					} else {
						tns.dirty = true;
					}
					if (!resized || win.fullscreen) {
						redraw();
						set_timeout(clear_resize, TO_REDRAW_RESIZE, false);
						resized = true;
					} else {
						set_timeout(redraw, TO_REDRAW_RESIZE, false);
					}
				}
				break;
			case KeyPress:
				on_keypress(&ev.xkey);
				break;
			case MotionNotify:
				if (mode == MODE_IMAGE) {
					win_set_cursor(&win, CURSOR_ARROW);
					set_timeout(reset_cursor, TO_CURSOR_HIDE, true);
				}
				break;
		}
	}
}

int fncmp(const void *a, const void *b)
{
	return strcoll(((fileinfo_t*) a)->name, ((fileinfo_t*) b)->name);
}

int main(int argc, char **argv)
{
	int i, start;
	size_t n;
	ssize_t len;
	char *filename;
	const char *homedir, *dsuffix = "";
	struct stat fstats;
	r_dir_t dir;

	parse_options(argc, argv);

	if (options->clean_cache) {
		tns_init(&tns, 0, NULL);
		tns_clean_cache(&tns);
		exit(EXIT_SUCCESS);
	}

	if (options->filecnt == 0 && !options->from_stdin) {
		print_usage();
		exit(EXIT_FAILURE);
	}

	if (options->recursive || options->from_stdin)
		filecnt = FILENAME_CNT;
	else
		filecnt = options->filecnt;

	files = (fileinfo_t*) s_malloc(filecnt * sizeof(fileinfo_t));
	fileidx = 0;

	if (options->from_stdin) {
		filename = NULL;
		while ((len = get_line(&filename, &n, stdin)) > 0) {
			if (filename[len-1] == '\n')
				filename[len-1] = '\0';
			check_add_file(filename);
		}
		if (filename != NULL)
			free(filename);
	}

	for (i = 0; i < options->filecnt; i++) {
		filename = options->filenames[i];

		if (stat(filename, &fstats) < 0) {
			warn("could not stat file: %s", filename);
			continue;
		}
		if (!S_ISDIR(fstats.st_mode)) {
			check_add_file(filename);
		} else {
			if (!options->recursive) {
				warn("ignoring directory: %s", filename);
				continue;
			}
			if (r_opendir(&dir, filename) < 0) {
				warn("could not open directory: %s", filename);
				continue;
			}
			start = fileidx;
			while ((filename = r_readdir(&dir)) != NULL) {
				check_add_file(filename);
				free((void*) filename);
			}
			r_closedir(&dir);
			if (fileidx - start > 1)
				qsort(files + start, fileidx - start, sizeof(fileinfo_t), fncmp);
		}
	}

	if (fileidx == 0) {
		fprintf(stderr, "sxiv: no valid image file given, aborting\n");
		exit(EXIT_FAILURE);
	}

	filecnt = fileidx;
	fileidx = options->startnum < filecnt ? options->startnum : 0;

	win_init(&win);
	img_init(&img, &win);

	if ((homedir = getenv("XDG_CONFIG_HOME")) == NULL || homedir[0] == '\0') {
		homedir = getenv("HOME");
		dsuffix = "/.config";
	}
	if (homedir != NULL) {
		char **cmd[] = { &info.cmd, &keyhandler.cmd };
		const char *name[] = { "image-info", "key-handler" };

		for (i = 0; i < ARRLEN(cmd); i++) {
			len = strlen(homedir) + strlen(dsuffix) + strlen(name[i]) + 12;
			*cmd[i] = (char*) s_malloc(len);
			snprintf(*cmd[i], len, "%s%s/sxiv/exec/%s", homedir, dsuffix, name[i]);
			if (access(*cmd[i], X_OK) != 0) {
				free(*cmd[i]);
				*cmd[i] = NULL;
			}
		}
	} else {
		warn("could not locate exec directory");
	}
	info.fd = -1;

	if (options->thumb_mode) {
		mode = MODE_THUMB;
		tns_init(&tns, filecnt, &win);
		while (!tns_load(&tns, 0, &files[0], false, false))
			remove_file(0, false);
		tns.cnt = 1;
	} else {
		mode = MODE_IMAGE;
		tns.thumbs = NULL;
		load_image(fileidx);
	}

	win_open(&win);
	win_set_cursor(&win, CURSOR_WATCH);

	run();
	cleanup();

	return 0;
}
