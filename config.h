#ifdef _GENERAL_CONFIG

/* enable external commands (defined below)? 0 = off, 1 = on:  */
enum { EXT_COMMANDS = 0 };

#endif
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
	/* key              function            argument */
	{ XK_q,             quit,               None },
	{ XK_r,             reload,             None },
	{ XK_f,             toggle_fullscreen,  None },
	{ XK_a,             toggle_antialias,   None },
	{ XK_A,             toggle_alpha,       None },
	{ XK_Return,        switch_mode,        None },

	{ XK_g,             first,              None },
	{ XK_G,             last,               None },
	{ XK_n,             navigate,           +1 },
	{ XK_space,         navigate,           +1 },
	{ XK_p,             navigate,           -1 },
	{ XK_BackSpace,     navigate,           -1 },
	{ XK_bracketright,  navigate,           +10 },
	{ XK_bracketleft,   navigate,           -10 },

	{ XK_D,             remove_image,       None },

	{ XK_h,             move,               DIR_LEFT },
	{ XK_Left,          move,               DIR_LEFT },
	{ XK_j,             move,               DIR_DOWN },
	{ XK_Down,          move,               DIR_DOWN },
	{ XK_k,             move,               DIR_UP },
	{ XK_Up,            move,               DIR_UP },
	{ XK_l,             move,               DIR_RIGHT },
	{ XK_Right,         move,               DIR_RIGHT },

	{ XK_braceleft,     scroll,             DIR_LEFT },
	{ XK_Next,          scroll,             DIR_DOWN },
	{ XK_Prior,         scroll,             DIR_UP },
	{ XK_braceright,    scroll,             DIR_RIGHT },

	{ XK_H,             pan_edge,           DIR_LEFT },
	{ XK_J,             pan_edge,           DIR_DOWN },
	{ XK_K,             pan_edge,           DIR_UP },
	{ XK_L,             pan_edge,           DIR_RIGHT },

	{ XK_plus,          zoom,               +1 },
	{ XK_equal,         zoom,               +1 },
	{ XK_KP_Add,        zoom,               +1 },
	{ XK_minus,         zoom,               -1 },
	{ XK_KP_Subtract,   zoom,               -1 },
	{ XK_0,             zoom,               0 },
	{ XK_KP_0,          zoom,               0 },
	{ XK_w,             fit_to_win,         None },
	{ XK_W,             fit_to_img,         None },

	{ XK_less,          rotate,             DIR_LEFT },
	{ XK_greater,       rotate,             DIR_RIGHT },
};

/* external commands and corresponding key mappings:           */
static const command_t commands[] = {
	/* ctrl-...    reload?  command, '#' is replaced by filename */
	{ XK_comma,    True,    "jpegtran -rotate 270 -copy all -outfile # #" },
	{ XK_period,   True,    "jpegtran -rotate 90 -copy all -outfile # #" },
	{ XK_less,     True,    "mogrify -rotate -90 #" },
	{ XK_greater,  True,    "mogrify -rotate +90 #" }
};

/* mouse button mappings for image mode:                       */
static const button_t buttons[] = {
	/* modifier     button       function    argument            */
	{ None,         Button1,     navigate,   +1 },
	{ None,         Button3,     navigate,   -1 },
	{ None,         Button2,     drag,       None },
	{ None,         Button4,     move,       DIR_UP },
	{ None,         Button5,     move,       DIR_DOWN },
	{ ShiftMask,    Button4,     move,       DIR_LEFT },
	{ ShiftMask,    Button5,     move,       DIR_RIGHT },
	{ ControlMask,  Button4,     zoom,       +1 },
	{ ControlMask,  Button5,     zoom,       -1 },
};

#endif
