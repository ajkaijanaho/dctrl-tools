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

#ifndef PARAGRAPH_H
#define PARAGRAPH_H

#define MAX_FIELDS 100

#include <stddef.h>
#include "fsaf.h"
#include "fieldtrie.h"

struct field_data {
	size_t start, end; /* offsets to the file; [start,end) is the body */
};

struct paragraph_private {
	FSAF * fp;
	fieldtrie_t * trie;
	bool eof;

	// CURRENT PARAGRAPH PARSED

	size_t start, end; /* offsets to the file; [start,end) is the paragraph */
	struct field_data fields[MAX_FIELDS];
};

typedef struct paragraph_private para_t;

/* Initialize the given para_t, associating with it the given
 * FSAF and the field trie.  */
void para_init(para_t *, FSAF *, fieldtrie_t *);

void para_parse_next(para_t *);

static inline
bool para_eof(para_t * para) { return para->eof; }

#endif /* PARAGRAPH_H */
