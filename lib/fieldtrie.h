/*  dctrl-tools - Debian control file inspection tools
    Copyright (C) 2003, 2004 Antti-Juhani Kaijanaho

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
#include <stdbool.h>

struct field_attr {
	bool valid;
	size_t inx;
};

void fieldtrie_init(void);

// case-insensitive
size_t fieldtrie_insert(char const *);

// case-insensitive
struct field_attr fieldtrie_lookup(char const *, size_t n);

//void fieldtrie_clear(void);

size_t fieldtrie_count(void);

#endif /* FIELDTRIE_H */
