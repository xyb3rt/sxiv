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

#ifndef SXIV_H
#define SXIV_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <Imlib2.h>
#include <X11/Xlib.h>

/*
 * Annotation for functions called in cleanup().
 * These functions are not allowed to call error(!0, ...) or exit().
 */
#define CLEANUP

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#define ARRLEN(a) (sizeof(a) / sizeof((a)[0]))

#define STREQ(s1,s2) (strcmp((s1), (s2)) == 0)

#define TV_DIFF(t1,t2) (((t1)->tv_sec  - (t2)->tv_sec ) * 1000 + \
                        ((t1)->tv_usec - (t2)->tv_usec) / 1000)

#define TV_SET_MSEC(tv,t) {             \
  (tv)->tv_sec  = (t) / 1000;           \
  (tv)->tv_usec = (t) % 1000 * 1000;    \
}

#define TV_ADD_MSEC(tv,t) {             \
  (tv)->tv_sec  += (t) / 1000;          \
  (tv)->tv_usec += (t) % 1000 * 1000;   \
}

typedef enum {
	BO_BIG_ENDIAN,
	BO_LITTLE_ENDIAN
} byteorder_t;

typedef enum {
	MODE_IMAGE,
	MODE_THUMB
} appmode_t;

typedef enum {
	DIR_LEFT  = 1,
	DIR_RIGHT = 2,
	DIR_UP    = 4,
	DIR_DOWN  = 8
} direction_t;

typedef enum {
	DEGREE_90  = 1,
	DEGREE_180 = 2,
	DEGREE_270 = 3
} degree_t;

typedef enum {
	FLIP_HORIZONTAL = 1,
	FLIP_VERTICAL   = 2
} flipdir_t;

typedef enum {
	SCALE_DOWN,
	SCALE_FIT,
	SCALE_WIDTH,
	SCALE_HEIGHT,
	SCALE_ZOOM
} scalemode_t;

typedef enum {
	DRAG_RELATIVE,
	DRAG_ABSOLUTE
} dragmode_t;

typedef enum {
	CURSOR_ARROW,
	CURSOR_DRAG,
	CURSOR_WATCH,
	CURSOR_LEFT,
	CURSOR_RIGHT,
	CURSOR_NONE,

	CURSOR_COUNT
} cursor_t;

typedef enum {
	FF_WARN    = 1,
	FF_MARK    = 2,
	FF_TN_INIT = 4
} fileflags_t;

typedef struct {
	const char *name; /* as given by user */
	const char *path; /* always absolute */
	fileflags_t flags;
} fileinfo_t;

/* timeouts in milliseconds: */
enum {
	TO_REDRAW_RESIZE = 75,
	TO_REDRAW_THUMBS = 200,
	TO_CURSOR_HIDE   = 1200,
	TO_DOUBLE_CLICK  = 300
};

typedef void (*timeout_f)(void);

typedef struct arl arl_t;
typedef struct img img_t;
typedef struct opt opt_t;
typedef struct tns tns_t;
typedef struct win win_t;


/* autoreload.c */

struct arl {
	int fd;
	int wd_dir;
	int wd_file;
	char *filename;
};

void arl_init(arl_t*);
void arl_cleanup(arl_t*);
void arl_setup(arl_t*, const char* /* result of realpath(3) */);
bool arl_handle(arl_t*);


/* commands.c */

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

extern const cmd_t cmds[CMD_COUNT];


/* image.c */

typedef struct {
	Imlib_Image im;
	unsigned int delay;
} img_frame_t;

typedef struct {
	img_frame_t *frames;
	int cap;
	int cnt;
	int sel;
	bool animate;
	int framedelay;
	int length;
} multi_img_t;

struct img {
	Imlib_Image im;
	int w;
	int h;

	win_t *win;
	float x;
	float y;

	scalemode_t scalemode;
	float zoom;

	bool checkpan;
	bool dirty;
	bool aa;
	bool alpha;

	Imlib_Color_Modifier cmod;
	int gamma;

	struct {
		bool on;
		int delay;
	} ss;

	multi_img_t multi;
};

void img_init(img_t*, win_t*);
bool img_load(img_t*, const fileinfo_t*);
CLEANUP void img_close(img_t*, bool);
void img_render(img_t*);
bool img_fit_win(img_t*, scalemode_t);
bool img_zoom(img_t*, float);
bool img_zoom_in(img_t*);
bool img_zoom_out(img_t*);
bool img_pos(img_t*, float, float);
bool img_move(img_t*, float, float);
bool img_pan(img_t*, direction_t, int);
bool img_pan_edge(img_t*, direction_t);
void img_rotate(img_t*, degree_t);
void img_flip(img_t*, flipdir_t);
void img_toggle_antialias(img_t*);
bool img_change_gamma(img_t*, int);
bool img_frame_navigate(img_t*, int);
bool img_frame_animate(img_t*);


/* options.c */

struct opt {
	/* file list: */
	char **filenames;
	bool from_stdin;
	bool to_stdout;
	bool recursive;
	int filecnt;
	int startnum;

	/* image: */
	scalemode_t scalemode;
	float zoom;
	bool animate;
	int gamma;
	int slideshow;
	int framerate;

	/* window: */
	bool fullscreen;
	bool hide_bar;
	long embed;
	char *geometry;
	char *res_name;

	/* misc flags: */
	bool quiet;
	bool thumb_mode;
	bool clean_cache;
	bool private_mode;
};

extern const opt_t *options;

void print_usage(void);
void print_version(void);
void parse_options(int, char**);


/* thumbs.c */

typedef struct {
	Imlib_Image im;
	int w;
	int h;
	int x;
	int y;
} thumb_t;

struct tns {
	fileinfo_t *files;
	thumb_t *thumbs;
	const int *cnt;
	int *sel;
	int initnext;
	int loadnext;
	int first, end;
	int r_first, r_end;

	win_t *win;
	int x;
	int y;
	int cols;
	int rows;
	int zl;
	int bw;
	int dim;

	bool dirty;
};

void tns_clean_cache(tns_t*);
void tns_init(tns_t*, fileinfo_t*, const int*, int*, win_t*);
CLEANUP void tns_free(tns_t*);
bool tns_load(tns_t*, int, bool, bool);
void tns_unload(tns_t*, int);
void tns_render(tns_t*);
void tns_mark(tns_t*, int, bool);
void tns_highlight(tns_t*, int, bool);
bool tns_move_selection(tns_t*, direction_t, int);
bool tns_scroll(tns_t*, direction_t, bool);
bool tns_zoom(tns_t*, int);
int tns_translate(tns_t*, int, int);


/* util.c */

#include <dirent.h>

typedef struct {
	DIR *dir;
	char *name;
	int d;
	bool recursive;

	char **stack;
	int stcap;
	int stlen;
} r_dir_t;

extern const char *progname;

void* emalloc(size_t);
void* erealloc(void*, size_t);
char* estrdup(const char*);
void error(int, int, const char*, ...);
void size_readable(float*, const char**);
int r_opendir(r_dir_t*, const char*, bool);
int r_closedir(r_dir_t*);
char* r_readdir(r_dir_t*, bool);
int r_mkdir(char*);


/* window.c */

#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

enum {
	BAR_L_LEN = 512,
	BAR_R_LEN = 64
};

enum {
	ATOM_WM_DELETE_WINDOW,
	ATOM__NET_WM_NAME,
	ATOM__NET_WM_ICON_NAME,
	ATOM__NET_WM_ICON,
	ATOM__NET_WM_STATE,
	ATOM__NET_WM_STATE_FULLSCREEN,
	ATOM_COUNT
};

typedef struct {
	Display *dpy;
	int scr;
	int scrw, scrh;
	Visual *vis;
	Colormap cmap;
	int depth;
} win_env_t;

typedef struct {
	size_t size;
	char *p;
	char *buf;
} win_bar_t;

struct win {
	Window xwin;
	win_env_t env;

	XftColor bg;
	XftColor fg;

	int x;
	int y;
	unsigned int w;
	unsigned int h; /* = win height - bar height */
	unsigned int bw;

	struct {
		int w;
		int h;
		Pixmap pm;
	} buf;

	struct {
		unsigned int h;
		win_bar_t l;
		win_bar_t r;
	} bar;
};

extern Atom atoms[ATOM_COUNT];

void win_init(win_t*);
void win_open(win_t*);
CLEANUP void win_close(win_t*);
bool win_configure(win_t*, XConfigureEvent*);
void win_toggle_fullscreen(win_t*);
void win_toggle_bar(win_t*);
void win_clear(win_t*);
void win_draw(win_t*);
void win_draw_rect(win_t*, int, int, int, int, bool, int, unsigned long);
void win_set_title(win_t*, const char*);
void win_set_cursor(win_t*, cursor_t);
void win_cursor_pos(win_t*, int*, int*);

#endif /* SXIV_H */

