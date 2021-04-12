/*
 * This file contains patch control flags.
 *
 * In principle you should be able to mix and match any patches
 * you may want. In cases where patches are logically incompatible
 * one patch may take precedence over the other as noted in the
 * relevant descriptions.
 */

/* Sets the _NET_WM_PID X property.
 *
 * Without this using tools like wmctrl -lp the PID for sxiv windows are displayed
 * as 0 indicating that "the application owning the window does not support it."
 *
 * https://github.com/muennich/sxiv/issues/398
 * https://github.com/muennich/sxiv/pull/403
 */
#define EWMH_NET_WM_PID_PATCH 0

/* Sets the WM_CLIENT_MACHINE X property.
 * https://github.com/muennich/sxiv/pull/403
 */
#define EWMH_WM_CLIENT_MACHINE 0

/* Adds the ability to cycle when viewing multiple images.
 * https://github.com/i-tsvetkov/sxiv-patches
 */
#define IMAGE_MODE_CYCLE_PATCH 0

/* Makes thumbnails square.
 * https://github.com/i-tsvetkov/sxiv-patches
 */
#define SQUARE_THUMBNAILS_PATCH 0

/* Adds support for SVG image format (2021-03-14).
 *
 * This patch depends on the following libraries for loading svg documents:
 *    - librsvg
 *    - cairo
 *
 * Remember to uncomment the relevant lines in the Makefile when enabling this patch.
 *
 * Known issues:
 *    - the vector images are converted to raster and thus do not retain their scalable nature
 *
 * https://github.com/muennich/sxiv/pull/440
 */
#define SVG_IMAGE_SUPPORT_PATCH 0

/* Adds support for animated WebP image format.
 * Note that as-is sxiv has support for still WebP images through the Imlib2 library. This
 * only adds support for animated WebP images (similar to animated GIF images).
 *
 * This patch depends on the following libraries for loading svg documents:
 *    - lwebp
 *    - lwebpdemux
 *
 * Remember to uncomment the relevant lines in the Makefile when enabling this patch.
 *
 * Known issues:
 *    - the rendering of transparency for still WebP images is different to what sxiv has
 *      by default when rendering WebP images (without this patch)
 *
 * https://github.com/muennich/sxiv/pull/437
 */
#define WEBP_IMAGE_SUPPORT_PATCH 0

/* Makes the window size fit the image when displaying a single image.
 * https://github.com/i-tsvetkov/sxiv-patches
 */
#define WINDOW_FIT_IMAGE_PATCH 0

/*
 * With this patch when the window is mapped, some ICCCM WM_HINTS are set.
 * The input field is set to true and state is set to NormalState.
 *
 * To quote the spec, "The input field is used to communicate to the window
 * manager the input focus model used by the client" and "[c]lients with
 * the Passive and Locally Active models should set the input flag to
 * True". sxiv falls under the Passive Input model, since it expects keyboard
 * input, but only listens for key events on its single, top-level window instead
 * of subordinate windows (Locally Active) or the root window (Globally Active).
 *
 * From the end users prospective, all EWMH/ICCCM compliant WMs (especially
 * the minimalistic ones) will allow the user to focus sxiv, which will
 * allow sxiv to receive key events. If the input field is not set, WMs are
 * allowed to assume that sxiv doesn't require focus.
 *
 * https://github.com/muennich/sxiv/pull/406
 */
#define WM_HINTS_PATCH 0
