/* default window dimensions:                            */
#define WIN_WIDTH  800
#define WIN_HEIGHT 600

/* default color to use for window background:           *
 *   (see X(7) "COLOR NAMES" section for valid values)   */
#define BG_COLOR   "#888888"

/* default scale mode when loading a new image:          *
 *   SCALE_DOWN: 100%, but fit large images into window, *
 *   SCALE_FIT:  fit all images into window,             *
 *   SCALE_ZOOM: use current zoom level, 100% at startup */
#define SCALE_MODE SCALE_DOWN

/* levels (percent) to use when zooming via '-' and '+': */
static const float zoom_levels[] = {
	 12.5,  25.0,  50.0,  75.0,
	100.0, 150.0, 200.0, 400.0, 800.0
};
