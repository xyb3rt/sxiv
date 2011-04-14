/* default window dimensions (overwritten via -g option): */
#define WIN_WIDTH    800
#define WIN_HEIGHT   600

/* default color for window background:                   *
 * (see X(7) "COLOR NAMES" section for valid values)      */
#define BG_COLOR     "#999999"
/* default color for thumbnail selection:                 */
#define SEL_COLOR    "#0040FF"

/* how should images be scaled when they are loaded?:     *
 * (also controllable via -d/-s/-Z/-z options)            *
 *   SCALE_DOWN: 100%, but fit large images into window,  *
 *   SCALE_FIT:  fit all images into window,              *
 *   SCALE_ZOOM: use current zoom level, 100% at startup  */
#define SCALE_MODE   SCALE_DOWN

/* levels (percent) to use when zooming via '-' and '+':  */
static const float zoom_levels[] = {
	 12.5,  25.0,  50.0,  75.0,
	100.0, 150.0, 200.0, 400.0, 800.0
};

/* default dimension of thumbnails (width == height):     */
#define THUMB_SIZE   60

/* enable external commands (defined below)? 0=off, 1=on: */
#define EXT_COMMANDS 0

/* external commands and corresponding key mappings:      */
#ifdef MAIN_C
#if    EXT_COMMANDS
static const command_t commands[] = {
	/* ctrl-...     reload?  command, '#' is replaced by filename */
	{  XK_comma,    True,    "jpegtran -rotate 270 -copy all -outfile # #" },
	{  XK_period,   True,    "jpegtran -rotate 90 -copy all -outfile # #" },
	{  XK_less,     True,    "mogrify -rotate -90 #" },
	{  XK_greater,  True,    "mogrify -rotate +90 #" }
};
#endif
#endif
