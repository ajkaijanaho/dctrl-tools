/*  dctrl-tools - Debian control file inspection tools
    Copyright © 2003, 2004 Antti-Juhani Kaijanaho

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef FIELDTRIE_H
#define FIELDTRIE_H

#include <limits.h>
#include <stddef.h>
#include <stdbool.h>

struct field_attr {
	char const *const name;
	const size_t namelen;
	const size_t inx;
        _Bool is_show_field; /* whether this field is (globally) selected
                                for showing */
        size_t backup_field; /* index to field whose value should be
                                used if this field is empty, or
                                (size_t)-1 */
};

void fieldtrie_init(void);

// case-insensitive
struct field_attr *fieldtrie_insert(char const *);
struct field_attr *fieldtrie_insert_n(char const * s, size_t slen);

// case-insensitive
struct field_attr *fieldtrie_lookup(char const *, size_t n);

struct field_attr *fieldtrie_get(size_t inx);

//void fieldtrie_clear(void);

size_t fieldtrie_count(void);

#endif /* FIELDTRIE_H */
