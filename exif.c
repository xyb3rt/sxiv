/* sxiv: exif.c
 * Copyright (c) 2012 Bert Muennich <be.muennich at googlemail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "exif.h"
#include "util.h"

ssize_t s_read(int fd, const char *fn, void *buf, size_t n) {
	ssize_t ret;

	ret = read(fd, buf, n);
	if (ret < n) {
		warn("unexpected end-of-file: %s", fn);
		return -1;
	} else {
		return ret;
	}
}

unsigned short btous(unsigned char *buf, byteorder_t order) {
	if (buf == NULL)
		return 0;
	if (order == BO_BIG_ENDIAN)
		return buf[0] << 8 | buf[1];
	else
		return buf[1] << 8 | buf[0];
}

unsigned int btoui(unsigned char *buf, byteorder_t order) {
	if (buf == NULL)
		return 0;
	if (order == BO_BIG_ENDIAN)
		return buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
	else
		return buf[3] << 24 | buf[2] << 16 | buf[1] << 8 | buf[0];
}

int exif_orientation(const fileinfo_t *file) {
	int fd;
	unsigned char data[EXIF_MAX_LEN];
	byteorder_t order = BO_BIG_ENDIAN;
	unsigned int cnt, len, idx, val;

	if (file == NULL || file->path == NULL)
		return -1;

	fd = open(file->path, O_RDONLY);
	if (fd < 0)
		return -1;

	if (s_read(fd, file->name, data, 4) < 0)
		goto abort;
	if (btous(data, order) != JPEG_MARKER_SOI)
		goto abort;
	if (btous(data + 2, order) != JPEG_MARKER_APP1)
		goto abort;

	if (s_read(fd, file->name, data, 2) < 0)
		goto abort;
	len = btous(data, order);
	if (len < 8)
		goto abort;

	if (s_read(fd, file->name, data, 6) < 0)
		goto abort;
	if (btoui(data, order) != EXIF_HEAD)
		goto abort;

	len -= 8;
	if (len < 12 || len > EXIF_MAX_LEN)
		goto abort;
	if (s_read(fd, file->name, data, len) < 0)
		goto abort;

	switch (btous(data, order)) {
		case EXIF_BO_BIG_ENDIAN:
			order = BO_BIG_ENDIAN;
			break;
		case EXIF_BO_LITTLE_ENDIAN:
			order = BO_LITTLE_ENDIAN;
			break;
		default:
			goto abort;
			break;
	}

	if (btous(data + 2, order) != EXIF_TAG_MARK)
		goto abort;
	idx = btoui(data + 4, order);
	if (idx > len - 2)
		goto abort;

	val = 0;
	cnt = btous(data + idx, order);

	for (idx += 2; cnt > 0 && idx < len - 12; cnt--, idx += 12) {
		if (btous(data + idx, order) == EXIF_TAG_ORIENTATION) {
			val = btous(data + idx + 8, order);
			break;
		}
	}

	close(fd);
	return val;

abort:
	close(fd);
	return -1;
}
