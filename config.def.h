#ifdef _WINDOW_CONFIG

/* default window dimensions (overwritten via -g option): */
enum {
	WIN_WIDTH  = 800,
	WIN_HEIGHT = 600
};

/* bar font:
 * (see X(7) section "FONT NAMES" for valid values)
 */
static const char * const BAR_FONT = "-*-fixed-medium-r-*-*-13-*-*-*-*-60-*-*";

/* colors:
 * (see X(7) section "COLOR NAMES" for valid values)
 */
static const char * const WIN_BG_COLOR = "#777777";
static const char * const WIN_FS_COLOR = "#000000";
static const char * const SEL_COLOR    = "#DDDDDD";
static const char * const BAR_BG_COLOR = "#222222";
static const char * const BAR_FG_COLOR = "#EEEEEE";

#endif
#ifdef _IMAGE_CONFIG

/* levels (in percent) to use when zooming via '-' and '+':
 * (first/last value is used as min/max zoom level)
 */
static const float zoom_levels[] = {
	 12.5,  25.0,  50.0,  75.0,
	100.0, 150.0, 200.0, 400.0, 800.0
};

/* default slideshow delay (in sec, overwritten via -S option): */
enum { SLIDESHOW_DELAY = 5 };

/* default settings for multi-frame gif images: */
enum {
	GIF_DELAY    = 100, /* delay time (in ms) */
	GIF_AUTOPLAY = 1,   /* autoplay when loaded [0/1] */
	GIF_LOOP     = 0    /* loop? [0: no, 1: endless, -1: as specified in file] */
};

/* gamma correction: the user-visible ranges [-GAMMA_RANGE, 0] and
 * (0, GAMMA_RANGE] are mapped to the ranges [0, 1], and (1, GAMMA_MAX].
 * */
static const double GAMMA_MAX   = 10.0;
static const int    GAMMA_RANGE = 32;

/* if false, pixelate images at zoom level != 100%,
 * toggled with 'a' key binding
 */
static const bool ANTI_ALIAS = true;

/* if true, use a checkerboard background for alpha layer,
 * toggled with 'A' key binding
 */
static const bool ALPHA_LAYER = false;

#endif
#ifdef _THUMBS_CONFIG

/* default dimension of thumbnails (width == height): */
enum { THUMB_SIZE = 60 };

#endif
#ifdef _MAPPINGS_CONFIG

/* keyboard mappings for image and thumbnail mode: */
static const keymap_t keys[] = {
	/* modifiers    key               function              argument */
	{ 0,            XK_q,             g_quit,               (arg_t) None },
	{ 0,            XK_Return,        g_switch_mode,        (arg_t) None },
	{ 0,            XK_f,             g_toggle_fullscreen,  (arg_t) None },
	{ 0,            XK_b,             g_toggle_bar,         (arg_t) None },
	{ ControlMask,  XK_x,             g_prefix_external,    (arg_t) None },
	{ 0,            XK_g,             g_first,              (arg_t) None },
	{ 0,            XK_G,             g_n_or_last,          (arg_t) None },
	{ 0,            XK_r,             g_reload_image,       (arg_t) None },
	{ 0,            XK_D,             g_remove_image,       (arg_t) None },
	{ ControlMask,  XK_h,             g_scroll_screen,      (arg_t) DIR_LEFT },
	{ ControlMask,  XK_Left,          g_scroll_screen,      (arg_t) DIR_LEFT },
	{ ControlMask,  XK_j,             g_scroll_screen,      (arg_t) DIR_DOWN },
	{ ControlMask,  XK_Down,          g_scroll_screen,      (arg_t) DIR_DOWN },
	{ ControlMask,  XK_k,             g_scroll_screen,      (arg_t) DIR_UP },
	{ ControlMask,  XK_Up,            g_scroll_screen,      (arg_t) DIR_UP },
	{ ControlMask,  XK_l,             g_scroll_screen,      (arg_t) DIR_RIGHT },
	{ ControlMask,  XK_Right,         g_scroll_screen,      (arg_t) DIR_RIGHT },
	{ 0,            XK_m,             g_toggle_image_mark,  (arg_t) None },
	{ 0,            XK_M,             g_reverse_marks,      (arg_t) None },
	{ 0,            XK_N,             g_navigate_marked,    (arg_t) +1 },
	{ 0,            XK_P,             g_navigate_marked,    (arg_t) -1 },

	{ 0,            XK_h,             t_move_sel,           (arg_t) DIR_LEFT },
	{ 0,            XK_Left,          t_move_sel,           (arg_t) DIR_LEFT },
	{ 0,            XK_j,             t_move_sel,           (arg_t) DIR_DOWN },
	{ 0,            XK_Down,          t_move_sel,           (arg_t) DIR_DOWN },
	{ 0,            XK_k,             t_move_sel,           (arg_t) DIR_UP },
	{ 0,            XK_Up,            t_move_sel,           (arg_t) DIR_UP },
	{ 0,            XK_l,             t_move_sel,           (arg_t) DIR_RIGHT },
	{ 0,            XK_Right,         t_move_sel,           (arg_t) DIR_RIGHT },
	{ 0,            XK_R,             t_reload_all,         (arg_t) None },

	{ 0,            XK_n,             i_navigate,           (arg_t) +1 },
	{ 0,            XK_n,             i_scroll_to_edge,     (arg_t) (DIR_LEFT | DIR_UP) },
	{ 0,            XK_space,         i_navigate,           (arg_t) +1 },
	{ 0,            XK_p,             i_navigate,           (arg_t) -1 },
	{ 0,            XK_p,             i_scroll_to_edge,     (arg_t) (DIR_LEFT | DIR_UP) },
	{ 0,            XK_BackSpace,     i_navigate,           (arg_t) -1 },
	{ 0,            XK_bracketright,  i_navigate,           (arg_t) +10 },
	{ 0,            XK_bracketleft,   i_navigate,           (arg_t) -10 },
	{ ControlMask,  XK_6,             i_alternate,          (arg_t) None },
	{ ControlMask,  XK_n,             i_navigate_frame,     (arg_t) +1 },
	{ ControlMask,  XK_p,             i_navigate_frame,     (arg_t) -1 },
	{ ControlMask,  XK_space,         i_toggle_animation,   (arg_t) None },
	{ 0,            XK_h,             i_scroll,             (arg_t) DIR_LEFT },
	{ 0,            XK_Left,          i_scroll,             (arg_t) DIR_LEFT },
	{ 0,            XK_j,             i_scroll,             (arg_t) DIR_DOWN },
	{ 0,            XK_Down,          i_scroll,             (arg_t) DIR_DOWN },
	{ 0,            XK_k,             i_scroll,             (arg_t) DIR_UP },
	{ 0,            XK_Up,            i_scroll,             (arg_t) DIR_UP },
	{ 0,            XK_l,             i_scroll,             (arg_t) DIR_RIGHT },
	{ 0,            XK_Right,         i_scroll,             (arg_t) DIR_RIGHT },
	{ 0,            XK_H,             i_scroll_to_edge,     (arg_t) DIR_LEFT },
	{ 0,            XK_J,             i_scroll_to_edge,     (arg_t) DIR_DOWN },
	{ 0,            XK_K,             i_scroll_to_edge,     (arg_t) DIR_UP },
	{ 0,            XK_L,             i_scroll_to_edge,     (arg_t) DIR_RIGHT },
	{ 0,            XK_plus,          i_zoom,               (arg_t) +1 },
	{ 0,            XK_KP_Add,        i_zoom,               (arg_t) +1 },
	{ 0,            XK_minus,         i_zoom,               (arg_t) -1 },
	{ 0,            XK_KP_Subtract,   i_zoom,               (arg_t) -1 },
	{ 0,            XK_equal,         i_set_zoom,           (arg_t) 100 },
	{ 0,            XK_w,             i_fit_to_win,         (arg_t) SCALE_DOWN },
	{ 0,            XK_W,             i_fit_to_win,         (arg_t) SCALE_FIT },
	{ 0,            XK_e,             i_fit_to_win,         (arg_t) SCALE_WIDTH },
	{ 0,            XK_E,             i_fit_to_win,         (arg_t) SCALE_HEIGHT },
	{ 0,            XK_less,          i_rotate,             (arg_t) DEGREE_270 },
	{ 0,            XK_greater,       i_rotate,             (arg_t) DEGREE_90 },
	{ 0,            XK_question,      i_rotate,             (arg_t) DEGREE_180 },
	{ 0,            XK_bar,           i_flip,               (arg_t) FLIP_HORIZONTAL },
	{ 0,            XK_underscore,    i_flip,               (arg_t) FLIP_VERTICAL },
	{ 0,            XK_braceleft,     i_change_gamma,       (arg_t) -1 },
	{ 0,            XK_braceright,    i_change_gamma,       (arg_t) +1 },
	{ ControlMask,  XK_g,             i_change_gamma,       (arg_t)  0 },
	{ 0,            XK_a,             i_toggle_antialias,   (arg_t) None },
	{ 0,            XK_A,             i_toggle_alpha,       (arg_t) None },
	{ 0,            XK_s,             i_slideshow,          (arg_t) None },
};

/* mouse button mappings for image mode: */
static const button_t buttons[] = {
	/* modifiers    button            function              argument */
	{ 0,            1,                i_navigate,           (arg_t) +1 },
	{ 0,            3,                i_navigate,           (arg_t) -1 },
	{ 0,            2,                i_drag,               (arg_t) None },
	{ 0,            4,                i_scroll,             (arg_t) DIR_UP },
	{ 0,            5,                i_scroll,             (arg_t) DIR_DOWN },
	{ ShiftMask,    4,                i_scroll,             (arg_t) DIR_LEFT },
	{ ShiftMask,    5,                i_scroll,             (arg_t) DIR_RIGHT },
	{ 0,            6,                i_scroll,             (arg_t) DIR_LEFT },
	{ 0,            7,                i_scroll,             (arg_t) DIR_RIGHT },
	{ ControlMask,  4,                i_zoom,               (arg_t) +1 },
	{ ControlMask,  5,                i_zoom,               (arg_t) -1 },
};

#endif
