/* default window dimensions (overwritten via -g option): */
enum { WIN_WIDTH  = 800, WIN_HEIGHT = 600 };

/* default color for window background:                   *
 * (see X(7) "COLOR NAMES" section for valid values)      */
static const char * const BG_COLOR  = "#999999";
/* default color for thumbnail selection:                 */
static const char * const SEL_COLOR = "#0066FF";

/* how should images be scaled when they are loaded?:     *
 * (also controllable via -d/-s/-Z/-z options)            *
 *   SCALE_DOWN: 100%, but fit large images into window,  *
 *   SCALE_FIT:  fit all images into window,              *
 *   SCALE_ZOOM: use current zoom level, 100% at startup  */
static const scalemode_t SCALE_MODE = SCALE_DOWN;

/* levels (percent) to use when zooming via '-' and '+':  */
static const float zoom_levels[] = {
	 12.5,  25.0,  50.0,  75.0,
	100.0, 150.0, 200.0, 400.0, 800.0
};

/* default dimension of thumbnails (width == height):     */
enum { THUMB_SIZE = 60 };

/* enable external commands (defined below)? 0=off, 1=on: */
enum { EXT_COMMANDS = 0 };

/* external commands and corresponding key mappings:      */
static const command_t commands[] = {
	/* ctrl-...  reload?  command, '#' is replaced by filename */
	{  ',',      1,       "jpegtran -rotate 270 -copy all -outfile # #" },
	{  '.',      1,       "jpegtran -rotate 90 -copy all -outfile # #" },
	{  '<',      1,       "mogrify -rotate -90 #" },
	{  '>',      1,       "mogrify -rotate +90 #" }
};
