/*  dctrl-tools - Debian control file inspection tools
    Copyright © 2003, 2004, 2005, 2010, 2011 Antti-Juhani Kaijanaho

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

struct field_datum {
	size_t line;
	size_t start, end; /* offsets to the file; [start,end) is the body */
        size_t name_start, name_end; /* as start and end, but for the name */
        struct field_datum *next, *prev;
};

struct field_data {
        struct field_datum *first;
        struct field_datum *last;
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
	size_t nfields, maxfields;
	struct field_data * fields;
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
struct field_data find_field(const para_t *p, size_t fld_inx)
{
	return fld_inx < p->nfields
                ? p->fields[fld_inx]
                : (struct field_data){ NULL, NULL };
}

static inline
struct field_data find_field_wr(const para_t *p,
                                size_t fld_inx,
                                size_t repl_inx)
{
	struct field_data fd = find_field(p, fld_inx);
        if (fd.first == NULL && repl_inx != (size_t)(-1)) {
                fd = find_field(p, repl_inx);
        }
        return fd;
}

// NOTE: get_field finds the FIRST field of the name!
static inline
struct fsaf_read_rv get_field(para_t * p, size_t fld_inx, size_t repl_inx)
{
        struct field_data fds = find_field_wr(p, fld_inx, repl_inx);
        if (fds.first == NULL) {
                const struct fsaf_read_rv fail = {
                        .b = NULL,
                        .len = 0
                };
                return fail;
        }
        struct field_datum *fd = fds.first;
	return fsaf_read(p->common->fp, fd->start, fd->end - fd->start);
}

// NOTE: get_field_as finds the FIRST field of the name!
static inline
char * get_field_as(const para_t * p, size_t fld_inx)
{
	struct field_data fds = find_field(p, fld_inx);
        if (fds.first == NULL) return strdup("");
        struct field_datum *fd = fds.first;
	return fsaf_getas(p->common->fp, fd->start, fd->end - fd->start);
}


static inline
bool para_eof(para_parser_t * para) { return para->eof; }

#endif /* PARAGRAPH_H */
