/*
 * This file contains patch control flags.
 *
 * In principle you should be able to mix and match any patches
 * you may want. In cases where patches are logically incompatible
 * one patch may take precedence over the other as noted in the
 * relevant descriptions.
 */

/* Adds the ability to cycle when viewing multiple images.
 * https://github.com/i-tsvetkov/sxiv-patches
 */
#define IMAGE_MODE_CYCLE_PATCH 0

/* Makes thumbnails square.
 * https://github.com/i-tsvetkov/sxiv-patches
 */
#define SQUARE_THUMBNAILS_PATCH 0

/* Adds support for SVG image format.
 * https://github.com/muennich/sxiv/pull/440
 */
#define SVG_IMAGE_SUPPORT_PATCH 0

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
