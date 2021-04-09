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