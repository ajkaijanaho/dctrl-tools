/*  dctrl-tools - Debian control file inspection tools
    Copyright © 2003, 2004, 2005 Antti-Juhani Kaijanaho

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

#ifndef PARAGRAPH_H
#define PARAGRAPH_H

#define MAX_FIELDS 100

#include <stddef.h>
#include "fsaf.h"
#include "fieldtrie.h"

struct field_data {
	size_t line;
	size_t start, end; /* offsets to the file; [start,end) is the body */
};

struct paragraph_parser {
	bool eof;
	bool invalidate_p;
	FSAF * fp;
	size_t loc;
	size_t line;

	bool ignore_broken_paras;
        bool register_unknown_fields;
};

struct paragraph {
	size_t line;
	struct paragraph_parser * common;
	size_t start, end; /* offsets to the file; [start,end) is the paragraph */
	size_t nfields;
	struct field_data * fields;
	//struct field_data fields[MAX_FIELDS];
};

typedef struct paragraph_parser para_parser_t;
typedef struct paragraph para_t;

/* Initialize the given para_parser_t, associating with it the given FSAF. */
void para_parser_init(para_parser_t *, FSAF *,
		      bool invalidate_p, bool ignore_broken_paras,
                      bool register_unknown_fields);

/* Initialize the given para_t for the given parser. */
void para_init(para_parser_t *, para_t *);

void para_parse_next(para_t *);

static inline
struct fsaf_read_rv get_whole_para(para_t * p)
{
	return fsaf_read(p->common->fp, p->start, p->end - p->start);
}

static inline
struct fsaf_read_rv get_field(para_t * p, size_t fld_inx, size_t repl_inx)
{
	struct field_data * fd = &p->fields[fld_inx];
        if (fd->start == fd->end && repl_inx != (size_t)(-1)) {
                 fd = &p->fields[repl_inx];
        }
	return fsaf_read(p->common->fp, fd->start, fd->end - fd->start);
}

static inline
char * get_field_as(const para_t * p, size_t fld_inx)
{
	struct field_data * fd = &p->fields[fld_inx];
	return fsaf_getas(p->common->fp, fd->start, fd->end - fd->start);
}

static inline
bool para_eof(para_parser_t * para) { return para->eof; }

#endif /* PARAGRAPH_H */
