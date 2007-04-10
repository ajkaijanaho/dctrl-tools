/*  dctrl-tools - Debian control file inspection tools
    Copyright (C) 2004, 2005 Antti-Juhani Kaijanaho

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SORTER_H
#define SORTER_H

#include <stdbool.h>
#include "para_bundle.h"

struct key {
	int field_inx;
	enum { FT_STRING, FT_VERSION } type;
	bool reverse : 1;
};

struct keys {
	size_t max;
	size_t nks;
	struct key * keys;
};

typedef struct keys keys_t;

static inline
void keys_init(keys_t * ks)
{
	ks->max = 0;
	ks->nks = 0;
	ks->keys = 0;
}

static inline
void keys_append(keys_t * ks, struct key key)
{
	assert(ks != 0);
	if (ks->nks == ks->max) {
		size_t max = ks->max == 0 ? 4 : 2 * ks->max;
		struct key * keys = realloc(ks->keys, max * sizeof *keys);
		if (keys == 0) fatal_enomem(0);
		ks->max = max;
		ks->keys = keys;
	}
	assert(ks->nks < ks->max);
	ks->keys[ks->nks++] = key;
}

static inline
void keys_fini(keys_t * ks)
{
	assert(ks != 0);
	ks->max = 0;
	ks->nks = 0;
	free(ks->keys);
	ks->keys = 0;
}

int para_compare(keys_t *, const para_t *, const para_t *);

/*
  Note that this function is NOT reentrant!
*/
void sort_bundle(keys_t *, struct para_bundle *);


#endif /* SORTER_H */
