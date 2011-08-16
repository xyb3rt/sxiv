#ifdef _WINDOW_CONFIG

/* default window dimensions (overwritten via -g option):      */
enum { WIN_WIDTH  = 800, WIN_HEIGHT = 600 };

/* default color for window background:                        *
 * (see X(7) "COLOR NAMES" section for valid values)           */
static const char * const BG_COLOR  = "#777777";
/* default color for thumbnail selection:                      */
static const char * const SEL_COLOR = "#DDDDDD";

#endif
#ifdef _IMAGE_CONFIG

/* how should images be scaled when they are loaded?:          *
 * (also controllable via -d/-s/-Z/-z options)                 *
 *   SCALE_DOWN: 100%, but fit large images into window,       *
 *   SCALE_FIT:  fit all images into window,                   *
 *   SCALE_ZOOM: use current zoom level, 100% at startup       */
static const scalemode_t SCALE_MODE = SCALE_DOWN;

/* levels (percent) to use when zooming via '-' and '+':       */
static const float zoom_levels[] = {
	 12.5,  25.0,  50.0,  75.0,
	100.0, 150.0, 200.0, 400.0, 800.0
};

#endif
#ifdef _THUMBS_CONFIG

/* default dimension of thumbnails (width == height):          */
enum { THUMB_SIZE = 60 };

#endif
#ifdef _MAPPINGS_CONFIG

/* keyboard mappings for image and thumbnail mode:             */
static const keymap_t keys[] = {
	/* ctrl    key              function           argument      */
	{ False,   XK_q,            quit,              (arg_t) None },
	{ False,   XK_r,            reload,            (arg_t) None },
	{ False,   XK_f,            toggle_fullscreen, (arg_t) None },
	{ False,   XK_a,            toggle_antialias,  (arg_t) None },
	{ False,   XK_A,            toggle_alpha,      (arg_t) None },
	{ False,   XK_Return,       switch_mode,       (arg_t) None },

	{ False,   XK_g,            first,             (arg_t) None },
	{ False,   XK_G,            last,              (arg_t) None },
	{ False,   XK_n,            navigate,          (arg_t) +1 },
	{ False,   XK_space,        navigate,          (arg_t) +1 },
	{ False,   XK_p,            navigate,          (arg_t) -1 },
	{ False,   XK_BackSpace,    navigate,          (arg_t) -1 },
	{ False,   XK_bracketright, navigate,          (arg_t) +10 },
	{ False,   XK_bracketleft,  navigate,          (arg_t) -10 },

	{ False,   XK_D,            remove_image,      (arg_t) None },

	{ False,   XK_h,            move,              (arg_t) DIR_LEFT },
	{ False,   XK_Left,         move,              (arg_t) DIR_LEFT },
	{ False,   XK_j,            move,              (arg_t) DIR_DOWN },
	{ False,   XK_Down,         move,              (arg_t) DIR_DOWN },
	{ False,   XK_k,            move,              (arg_t) DIR_UP },
	{ False,   XK_Up,           move,              (arg_t) DIR_UP },
	{ False,   XK_l,            move,              (arg_t) DIR_RIGHT },
	{ False,   XK_Right,        move,              (arg_t) DIR_RIGHT },

	{ True,    XK_h,            pan_screen,        (arg_t) DIR_LEFT },
	{ True,    XK_Left,         pan_screen,        (arg_t) DIR_LEFT },
	{ True,    XK_j,            pan_screen,        (arg_t) DIR_DOWN },
	{ True,    XK_Down,         pan_screen,        (arg_t) DIR_DOWN },
	{ True,    XK_k,            pan_screen,        (arg_t) DIR_UP },
	{ True,    XK_Up,           pan_screen,        (arg_t) DIR_UP },
	{ True,    XK_l,            pan_screen,        (arg_t) DIR_RIGHT },
	{ True,    XK_Right,        pan_screen,        (arg_t) DIR_RIGHT },

	{ False,   XK_H,            pan_edge,          (arg_t) DIR_LEFT },
	{ False,   XK_J,            pan_edge,          (arg_t) DIR_DOWN },
	{ False,   XK_K,            pan_edge,          (arg_t) DIR_UP },
	{ False,   XK_L,            pan_edge,          (arg_t) DIR_RIGHT },

	{ False,   XK_plus,         zoom,              (arg_t) +1 },
	{ False,   XK_equal,        zoom,              (arg_t) +1 },
	{ False,   XK_KP_Add,       zoom,              (arg_t) +1 },
	{ False,   XK_minus,        zoom,              (arg_t) -1 },
	{ False,   XK_KP_Subtract,  zoom,              (arg_t) -1 },
	{ False,   XK_0,            zoom,              (arg_t) None },
	{ False,   XK_KP_0,         zoom,              (arg_t) None },
	{ False,   XK_w,            fit_to_win,        (arg_t) None },
	{ False,   XK_W,            fit_to_img,        (arg_t) None },

	{ False,   XK_less,         rotate,            (arg_t) DIR_LEFT },
	{ False,   XK_greater,      rotate,            (arg_t) DIR_RIGHT },

	                            /* open the current image with given program:  */
	{ True,    XK_g,            open_with,         (arg_t) "gimp" },

	                            /* run shell command line on the current file,
	                             * '#' is replaced by filename:                */
	{ True,    XK_less,         run_command,       (arg_t) "mogrify -rotate -90 #" },
	{ True,    XK_greater,      run_command,       (arg_t) "mogrify -rotate +90 #" },
	{ True,    XK_comma,        run_command,       (arg_t) "jpegtran -rotate 270 -copy all -outfile # #" },
	{ True,    XK_period,       run_command,       (arg_t) "jpegtran -rotate 90 -copy all -outfile # #" },
};

/* mouse button mappings for image mode:                       */
static const button_t buttons[] = {
	/* ctrl    shift    button       function      argument      */
	{ False,   False,   Button1,     navigate,     (arg_t) +1 },
	{ False,   False,   Button3,     navigate,     (arg_t) -1 },
	{ False,   False,   Button2,     drag,         (arg_t) None },
	{ False,   False,   Button4,     move,         (arg_t) DIR_UP },
	{ False,   False,   Button5,     move,         (arg_t) DIR_DOWN },
	{ False,   True,    Button4,     move,         (arg_t) DIR_LEFT },
	{ False,   True,    Button5,     move,         (arg_t) DIR_RIGHT },
	{ True,    False,   Button4,     zoom,         (arg_t) +1 },
	{ True,    False,   Button5,     zoom,         (arg_t) -1 },
};

#endif
