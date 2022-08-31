#ifdef _WINDOW_CONFIG

/* default window dimensions (overwritten via -g option): */
enum {
	WIN_WIDTH  = 800,
	WIN_HEIGHT = 600
};

/* colors and font are configured with 'background', 'foreground' and
 * 'font' X resource properties.
 * See X(7) section Resources and xrdb(1) for more information.
 */

#endif
#ifdef _IMAGE_CONFIG

/* levels (in percent) to use when zooming via '-' and '+':
 * (first/last value is used as min/max zoom level)
 */
/* exponentials from 1 to 1500, calculated with octave:
 * MAX_ZOOM = 1500;
 * x = linspace(0, log(MAX_ZOOM), 300);
 * y = exp(x);
 * for z = y
 * printf("%f, ", z);
 * end
 * printf("\n");
 */
static const float zoom_levels[] =
  {
   1.000000, 1.024761, 1.050134, 1.076136, 1.102782, 1.130087, 1.158069, 1.186743, 1.216127, 1.246239, 1.277097, 1.308718, 1.341123, 1.374330, 1.408359, 1.443230, 1.478966, 1.515585, 1.553112, 1.591568, 1.630976, 1.671360, 1.712744, 1.755152, 1.798610, 1.843145, 1.888782, 1.935549, 1.983474, 2.032586, 2.082914, 2.134488, 2.187339, 2.241499, 2.296999, 2.353874, 2.412157, 2.471884, 2.533089, 2.595809, 2.660083, 2.725948, 2.793444, 2.862611, 2.933490, 3.006125, 3.080558, 3.156834, 3.234999, 3.315099, 3.397183, 3.481299, 3.567498, 3.655831, 3.746351, 3.839112, 3.934171, 4.031583, 4.131407, 4.233703, 4.338531, 4.445955, 4.556040, 4.668849, 4.784452, 4.902918, 5.024317, 5.148721, 5.276206, 5.406848, 5.540724, 5.677915, 5.818503, 5.962572, 6.110209, 6.261500, 6.416538, 6.575415, 6.738226, 6.905067, 7.076040, 7.251247, 7.430791, 7.614781, 7.803327, 7.996542, 8.194540, 8.397441, 8.605366, 8.818439, 9.036788, 9.260543, 9.489839, 9.724812, 9.965604, 10.212357, 10.465220, 10.724344, 10.989884, 11.261999, 11.540852, 11.826610, 12.119442, 12.419526, 12.727040, 13.042167, 13.365098, 13.696025, 14.035145, 14.382662, 14.738784, 15.103724, 15.477700, 15.860936, 16.253660, 16.656109, 17.068523, 17.491148, 17.924238, 18.368051, 18.822853, 19.288917, 19.766520, 20.255949, 20.757496, 21.271462, 21.798155, 22.337888, 22.890985, 23.457778, 24.038604, 24.633812, 25.243758, 25.868806, 26.509330, 27.165715, 27.838352, 28.527643, 29.234002, 29.957851, 30.699622, 31.459760, 32.238720, 33.036967, 33.854979, 34.693245, 35.552267, 36.432559, 37.334648, 38.259073, 39.206387, 40.177157, 41.171963, 42.191402, 43.236082, 44.306629, 45.403684, 46.527902, 47.679956, 48.860536, 50.070348, 51.310115, 52.580579, 53.882501, 55.216659, 56.583851, 57.984896, 59.420631, 60.891916, 62.399630, 63.944677, 65.527979, 67.150485, 68.813165, 70.517013, 72.263050, 74.052320, 75.885893, 77.764865, 79.690363, 81.663536, 83.685567, 85.757664, 87.881067, 90.057046, 92.286904, 94.571974, 96.913624, 99.313254, 101.772301, 104.292234, 106.874562, 109.520830, 112.232621, 115.011558, 117.859302, 120.777558, 123.768071, 126.832631, 129.973071, 133.191270, 136.489153, 139.868693, 143.331912, 146.880883, 150.517727, 154.244622, 158.063797, 161.977536, 165.988182, 170.098133, 174.309848, 178.625848, 183.048714, 187.581093, 192.225695, 196.985300, 201.862756, 206.860980, 211.982962, 217.231767, 222.610535, 228.122484, 233.770912, 239.559198, 245.490804, 251.569280, 257.798262, 264.181478, 270.722744, 277.425976, 284.295183, 291.334475, 298.548064, 305.940264, 313.515500, 321.278301, 329.233314, 337.385297, 345.739127, 354.299803, 363.072444, 372.062301, 381.274751, 390.715307, 400.389615, 410.303464, 420.462784, 430.873655, 441.542304, 452.475114, 463.678626, 475.159543, 486.924733, 498.981235, 511.336262, 523.997206, 536.971641, 550.267329, 563.892226, 577.854482, 592.162450, 606.824691, 621.849977, 637.247296, 653.025860, 669.195110, 685.764719, 702.744599, 720.144910, 737.976061, 756.248720, 774.973820, 794.162563, 813.826429, 833.977181, 854.626877, 875.787870, 897.472819, 919.694699, 942.466803, 965.802757, 989.716520, 1014.222401, 1039.335059, 1065.069519, 1091.441178, 1118.465812, 1146.159589, 1174.539079, 1203.621259, 1233.423529, 1263.963717, 1295.260097, 1327.331390, 1360.196785, 1393.875943, 1428.389015, 1463.756647, 1500.000000
  };

/* default slideshow delay (in sec, overwritten via -S option): */
enum { SLIDESHOW_DELAY = 5 };

/* gamma correction: the user-visible ranges [-GAMMA_RANGE, 0] and
 * (0, GAMMA_RANGE] are mapped to the ranges [0, 1], and (1, GAMMA_MAX].
 * */
static const double GAMMA_MAX   = 10.0;
static const int    GAMMA_RANGE = 32;

/* command i_scroll pans image 1/PAN_FRACTION of screen width/height */
static const int PAN_FRACTION = 70;

/* if false, pixelate images at zoom level != 100%,
 * toggled with 'a' key binding
 */
static const bool ANTI_ALIAS = true;

/* if true, use a checkerboard background for alpha layer,
 * toggled with 'A' key binding
 */
static const bool ALPHA_LAYER = false;

/* fallback height and width for svg documents.
 * use these values in case svg document does not specify height and width.
 */
enum {
	FB_SVG_HEIGHT = 512,
	FB_SVG_WIDTH  = 512
};

#endif
#ifdef _THUMBS_CONFIG

/* thumbnail sizes in pixels (width == height): */
static const int thumb_sizes[] = { 32, 64, 96, 128, 160 };

/* thumbnail size at startup, index into thumb_sizes[]: */
static const int THUMB_SIZE = 3;

#endif
#ifdef _VIDEO_CONFIG
/*
 * command used to play videos
 */
// static const char *VIDEO_CMD = "xdg-open '%s'";

#endif
#ifdef _MAPPINGS_CONFIG

/* keyboard mappings for image and thumbnail mode: */
static const keymap_t keys[] = {
	/* modifiers    key               function              argument */
	{ 0,            XK_q,             g_quit,               None },
	{ 0,            XK_Return,        g_switch_mode,        None },
	{ 0,            XK_f,             g_toggle_fullscreen,  None },
	{ 0,            XK_b,             g_toggle_bar,         None },
	{ ControlMask,  XK_x,             g_prefix_external,    None },
	{ 0,            XK_g,             g_first,              None },
	{ 0,            XK_G,             g_n_or_last,          None },
	{ 0,            XK_r,             g_reload_image,       None },
	{ 0,            XK_D,             g_remove_image,       None },
	{ ControlMask,  XK_h,             g_scroll_screen,      DIR_LEFT },
	{ ControlMask,  XK_Left,          g_scroll_screen,      DIR_LEFT },
	{ ControlMask,  XK_j,             g_scroll_screen,      DIR_DOWN },
	{ ControlMask,  XK_Down,          g_scroll_screen,      DIR_DOWN },
	{ ControlMask,  XK_k,             g_scroll_screen,      DIR_UP },
	{ ControlMask,  XK_Up,            g_scroll_screen,      DIR_UP },
	{ ControlMask,  XK_l,             g_scroll_screen,      DIR_RIGHT },
	{ ControlMask,  XK_Right,         g_scroll_screen,      DIR_RIGHT },
	{ 0,            XK_plus,          g_zoom,               +1 },
	{ 0,            XK_KP_Add,        g_zoom,               +1 },
	{ 0,            XK_minus,         g_zoom,               -1 },
	{ 0,            XK_KP_Subtract,   g_zoom,               -1 },
	{ 0,            XK_m,             g_toggle_image_mark,  None },
	{ 0,            XK_M,             g_mark_range,         None },
	{ ControlMask,  XK_m,             g_reverse_marks,      None },
	{ ControlMask,  XK_u,             g_unmark_all,         None },
	{ 0,            XK_N,             g_navigate_marked,    +1 },
	{ 0,            XK_P,             g_navigate_marked,    -1 },
	{ 0,            XK_braceleft,     g_change_gamma,       -1 },
	{ 0,            XK_braceright,    g_change_gamma,       +1 },
	{ ControlMask,  XK_g,             g_change_gamma,        0 },

	{ 0,            XK_h,             t_move_sel,           DIR_LEFT },
	{ 0,            XK_Left,          t_move_sel,           DIR_LEFT },
	{ 0,            XK_j,             t_move_sel,           DIR_DOWN },
	{ 0,            XK_Down,          t_move_sel,           DIR_DOWN },
	{ 0,            XK_k,             t_move_sel,           DIR_UP },
	{ 0,            XK_Up,            t_move_sel,           DIR_UP },
	{ 0,            XK_l,             t_move_sel,           DIR_RIGHT },
	{ 0,            XK_Right,         t_move_sel,           DIR_RIGHT },
	{ 0,            XK_R,             t_reload_all,         None },

	{ 0,            XK_n,             i_navigate,           +1 },
	{ 0,            XK_n,             i_scroll_to_edge,     DIR_LEFT | DIR_UP },
	{ 0,            XK_space,         i_navigate,           +1 },
	{ 0,            XK_p,             i_navigate,           -1 },
	{ 0,            XK_p,             i_scroll_to_edge,     DIR_LEFT | DIR_UP },
	{ 0,            XK_BackSpace,     i_navigate,           -1 },
	{ 0,            XK_bracketright,  i_navigate,           +10 },
	{ 0,            XK_bracketleft,   i_navigate,           -10 },
	{ ControlMask,  XK_6,             i_alternate,          None },
	{ ControlMask,  XK_n,             i_navigate_frame,     +1 },
	{ ControlMask,  XK_p,             i_navigate_frame,     -1 },
	{ ControlMask,  XK_space,         i_toggle_animation,   None },
	{ 0,            XK_h,             i_scroll_or_navigate, DIR_LEFT },
	{ 0,            XK_Left,          i_scroll_or_navigate, DIR_LEFT },
	{ 0,            XK_j,             i_scroll_or_navigate, DIR_DOWN },
	{ 0,            XK_Down,          i_scroll_or_navigate, DIR_DOWN },
	{ 0,            XK_k,             i_scroll_or_navigate, DIR_UP },
	{ 0,            XK_Up,            i_scroll_or_navigate, DIR_UP },
	{ 0,            XK_l,             i_scroll_or_navigate, DIR_RIGHT },
	{ 0,            XK_Right,         i_scroll_or_navigate, DIR_RIGHT },
	{ 0,            XK_H,             i_scroll_to_edge,     DIR_LEFT },
	{ 0,            XK_J,             i_scroll_to_edge,     DIR_DOWN },
	{ 0,            XK_K,             i_scroll_to_edge,     DIR_UP },
	{ 0,            XK_L,             i_scroll_to_edge,     DIR_RIGHT },
	{ 0,            XK_equal,         i_set_zoom,           100 },
	{ 0,            XK_w,             i_fit_to_win,         SCALE_DOWN },
	{ 0,            XK_W,             i_fit_to_win,         SCALE_FIT },
	{ 0,            XK_e,             i_fit_to_win,         SCALE_WIDTH },
	{ 0,            XK_E,             i_fit_to_win,         SCALE_HEIGHT },
	{ 0,            XK_less,          i_rotate,             DEGREE_270 },
	{ 0,            XK_greater,       i_rotate,             DEGREE_90 },
	{ 0,            XK_question,      i_rotate,             DEGREE_180 },
	{ 0,            XK_bar,           i_flip,               FLIP_HORIZONTAL },
	{ 0,            XK_underscore,    i_flip,               FLIP_VERTICAL },
	{ 0,            XK_a,             i_toggle_antialias,   None },
	{ 0,            XK_A,             i_toggle_alpha,       None },
	{ 0,            XK_s,             i_slideshow,          None },
};

/* mouse button mappings for image mode: */
static const button_t buttons[] = {
	/* modifiers    button            function              argument       type */
	{ 0,            1,                i_cursor_navigate,    None,          ButtonRelease },
	{ 0,            1,                i_drag,               DRAG_RELATIVE, ButtonPress },
	{ 0,            3,                g_switch_mode,        None,          ButtonPress },
	{ 0,            4,                g_zoom,               +1,            ButtonPress },
	{ 0,            5,                g_zoom,               -1,            ButtonPress },
};

#endif
