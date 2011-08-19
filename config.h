#ifdef _WINDOW_CONFIG

/* default window dimensions (overwritten via -g option): */
enum {
	WIN_WIDTH  = 800,
	WIN_HEIGHT = 600
};

/* default color for window background: */
static const char * const BG_COLOR  = "#777777";
/* default color for thumbnail selection: */
static const char * const SEL_COLOR = "#DDDDDD";
/* (see X(7) section "COLOR NAMES" for valid values) */

#endif
#ifdef _IMAGE_CONFIG

/* how should images be scaled when they are loaded?
 * (also controllable via -d/-s/-Z/-z options)
 *   SCALE_DOWN: 100%, but fit large images into window,
 *   SCALE_FIT:  fit all images into window,
 *   SCALE_ZOOM: use current zoom level, 100% at startup
 */
static const scalemode_t SCALE_MODE = SCALE_DOWN;

/* levels (percent) to use when zooming via '-' and '+': */
static const float zoom_levels[] = {
	 12.5,  25.0,  50.0,  75.0,
	100.0, 150.0, 200.0, 400.0, 800.0
};

/* default settings for multi-frame gif images: */
enum {
	GIF_DELAY    = 100, /* delay time (in ms) */
	GIF_AUTOPLAY = 1,   /* autoplay when loaded [0/1] */
	GIF_LOOP     = 0    /* endless loop [0/1] */
};

#endif
#ifdef _THUMBS_CONFIG

/* default dimension of thumbnails (width == height): */
enum { THUMB_SIZE = 60 };

#endif
#ifdef _MAPPINGS_CONFIG

/* keyboard mappings for image and thumbnail mode: */
static const keymap_t keys[] = {
	/* ctrl   key               function              argument */
	{ False,  XK_q,             it_quit,              (arg_t) None },
	{ False,  XK_Return,        it_switch_mode,       (arg_t) None },
	{ False,  XK_f,             it_toggle_fullscreen, (arg_t) None },

	{ False,  XK_r,             it_reload_image,      (arg_t) None },
	{ False,  XK_D,             it_remove_image,      (arg_t) None },

	{ False,  XK_n,             i_navigate,           (arg_t) +1 },
	{ False,  XK_space,         i_navigate,           (arg_t) +1 },
	{ False,  XK_p,             i_navigate,           (arg_t) -1 },
	{ False,  XK_BackSpace,     i_navigate,           (arg_t) -1 },
	{ False,  XK_bracketright,  i_navigate,           (arg_t) +10 },
	{ False,  XK_bracketleft,   i_navigate,           (arg_t) -10 },
	{ False,  XK_g,             it_first,             (arg_t) None },
	{ False,  XK_G,             it_last,              (arg_t) None },

	{ True,   XK_n,             i_navigate_frame,     (arg_t) +1 },
	{ True,   XK_p,             i_navigate_frame,     (arg_t) -1 },
	{ True,   XK_space,         i_toggle_animation,   (arg_t) None },

	{ False,  XK_h,             it_move,              (arg_t) DIR_LEFT },
	{ False,  XK_Left,          it_move,              (arg_t) DIR_LEFT },
	{ False,  XK_j,             it_move,              (arg_t) DIR_DOWN },
	{ False,  XK_Down,          it_move,              (arg_t) DIR_DOWN },
	{ False,  XK_k,             it_move,              (arg_t) DIR_UP },
	{ False,  XK_Up,            it_move,              (arg_t) DIR_UP },
	{ False,  XK_l,             it_move,              (arg_t) DIR_RIGHT },
	{ False,  XK_Right,         it_move,              (arg_t) DIR_RIGHT },

	{ True,   XK_h,             i_pan_screen,         (arg_t) DIR_LEFT },
	{ True,   XK_Left,          i_pan_screen,         (arg_t) DIR_LEFT },
	{ True,   XK_j,             i_pan_screen,         (arg_t) DIR_DOWN },
	{ True,   XK_Down,          i_pan_screen,         (arg_t) DIR_DOWN },
	{ True,   XK_k,             i_pan_screen,         (arg_t) DIR_UP },
	{ True,   XK_Up,            i_pan_screen,         (arg_t) DIR_UP },
	{ True,   XK_l,             i_pan_screen,         (arg_t) DIR_RIGHT },
	{ True,   XK_Right,         i_pan_screen,         (arg_t) DIR_RIGHT },

	{ False,  XK_H,             i_pan_edge,           (arg_t) DIR_LEFT },
	{ False,  XK_J,             i_pan_edge,           (arg_t) DIR_DOWN },
	{ False,  XK_K,             i_pan_edge,           (arg_t) DIR_UP },
	{ False,  XK_L,             i_pan_edge,           (arg_t) DIR_RIGHT },

	{ False,  XK_plus,          i_zoom,               (arg_t) +1 },
	{ False,  XK_equal,         i_zoom,               (arg_t) +1 },
	{ False,  XK_KP_Add,        i_zoom,               (arg_t) +1 },
	{ False,  XK_minus,         i_zoom,               (arg_t) -1 },
	{ False,  XK_KP_Subtract,   i_zoom,               (arg_t) -1 },
	{ False,  XK_0,             i_zoom,               (arg_t) None },
	{ False,  XK_KP_0,          i_zoom,               (arg_t) None },
	{ False,  XK_w,             i_fit_to_win,         (arg_t) None },
	{ False,  XK_W,             i_fit_to_img,         (arg_t) None },

	{ False,  XK_less,          i_rotate,             (arg_t) DIR_LEFT },
	{ False,  XK_greater,       i_rotate,             (arg_t) DIR_RIGHT },

	{ False,  XK_a,             i_toggle_antialias,   (arg_t) None },
	{ False,  XK_A,             i_toggle_alpha,       (arg_t) None },

	/* open current image with given program: */
	{ True,   XK_g,             it_open_with,         (arg_t) "gimp" },

	/* run shell command line on current file ('#' is replaced by file path: */
	{ True,   XK_less,          it_shell_cmd,         (arg_t) "mogrify -rotate -90 #" },
	{ True,   XK_greater,       it_shell_cmd,         (arg_t) "mogrify -rotate +90 #" },
	{ True,   XK_comma,         it_shell_cmd,         (arg_t) "jpegtran -rotate 270 -copy all -outfile # #" },
	{ True,   XK_period,        it_shell_cmd,         (arg_t) "jpegtran -rotate 90 -copy all -outfile # #" },
};

/* mouse button mappings for image mode: */
static const button_t buttons[] = {
	/* ctrl   shift   button    function              argument */
	{ False,  False,  Button1,  i_navigate,           (arg_t) +1 },
	{ False,  False,  Button3,  i_navigate,           (arg_t) -1 },
	{ False,  False,  Button2,  i_drag,               (arg_t) None },
	{ False,  False,  Button4,  it_move,              (arg_t) DIR_UP },
	{ False,  False,  Button5,  it_move,              (arg_t) DIR_DOWN },
	{ False,  True,   Button4,  it_move,              (arg_t) DIR_LEFT },
	{ False,  True,   Button5,  it_move,              (arg_t) DIR_RIGHT },
	{ True,   False,  Button4,  i_zoom,               (arg_t) +1 },
	{ True,   False,  Button5,  i_zoom,               (arg_t) -1 },
};

#endif
