/* sxiv: sxiv.h
 * Copyright (c) 2011 Bert Muennich <muennich at informatik.hu-berlin.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef SXIV_H
#define SXIV_H

#include "config.h"
#include "options.h"

#define ABS(a)   ((a) < 0 ? (-(a)) : (a))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define WARN(...)                                                    \
  do {                                                               \
    if (!options->quiet) {                                           \
      fprintf(stderr, "sxiv: %s:%d: warning: ", __FILE__, __LINE__); \
      fprintf(stderr, __VA_ARGS__);                                  \
      fprintf(stderr, "\n");                                         \
    }                                                                \
  } while (0)

#define DIE(...)                                                     \
  do {                                                               \
    fprintf(stderr, "sxiv: %s:%d: error: ", __FILE__, __LINE__);     \
    fprintf(stderr, __VA_ARGS__);                                    \
    fprintf(stderr, "\n");                                           \
    cleanup();                                                       \
    exit(1);                                                         \
  } while (0)

void cleanup();

#endif /* SXIV_H */
