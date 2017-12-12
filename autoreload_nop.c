/* Copyright 2017 Max Voit
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

#include "sxiv.h"

void arl_init(arl_t *arl)
{
	arl->fd = -1;
}

void arl_cleanup(arl_t *arl)
{
	(void) arl;
}

void arl_setup(arl_t *arl, const char *filepath)
{
	(void) arl;
	(void) filepath;
}

bool arl_handle(arl_t *arl)
{ 
	(void) arl;
	return false;
}

