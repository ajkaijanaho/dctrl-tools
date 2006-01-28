/*  dctrl-tools - Debian control file inspection tools
    Copyright (C) 2003 Antti-Juhani Kaijanaho

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

#ifndef FIELDTRIE_H
#define FIELDTRIE_H

#include <limits.h>
#include <stddef.h>

struct field_bucket {
	char const * name;
	size_t namelen;
	size_t inx;
	struct field_bucket * next;
};

struct fieldtrie_private {
	size_t nextfree;
	/* A one-level trie listing all field names occurring in the
	 * atomic predicates. */
	struct field_bucket * fields[UCHAR_MAX];
};

typedef struct fieldtrie_private fieldtrie_t;

void fieldtrie_init(fieldtrie_t *);

// case-insensitive
size_t fieldtrie_insert(fieldtrie_t *, char const *);

// case-insensitive
// (size_t)(-1) if failed
size_t fieldtrie_lookup(fieldtrie_t *, char const *, size_t n);

void fieldtrie_clear(fieldtrie_t *);

#endif /* FIELDTRIE_H */
