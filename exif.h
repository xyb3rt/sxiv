/* Copyright 2012 Bert Muennich
 *
 * This file is part of sxiv.
 *
 * sxiv is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * sxiv is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with sxiv.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EXIF_H
#define EXIF_H

#include "types.h"

enum {
	JPEG_MARKER_SOI  = 0xFFD8,
	JPEG_MARKER_APP0 = 0xFFE0,
	JPEG_MARKER_APP1 = 0xFFE1
};

enum {
	EXIF_MAX_LEN          = 0x10000,
	EXIF_HEAD             = 0x45786966,
	EXIF_BO_BIG_ENDIAN    = 0x4D4D,
	EXIF_BO_LITTLE_ENDIAN = 0x4949,
	EXIF_TAG_MARK         = 0x002A,
	EXIF_TAG_ORIENTATION  = 0x0112
};

int exif_orientation(const fileinfo_t*);
void exif_auto_orientate(const fileinfo_t*); /* in image.c */

#endif /* EXIF_H */
