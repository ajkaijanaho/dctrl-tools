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

#include <string.h>
#include "msg.h"
#include "paragraph.h"
#include "strutil.h"

void para_parser_init(para_parser_t * pp, FSAF * fp,
		      bool invalidate_p, bool ignore_failing_paras,
                      bool register_unknown_fields)
{
	pp->fp = fp;
	pp->eof = false;
	pp->loc = 0;
	pp->line = 1;
	pp->invalidate_p = invalidate_p;
	pp->ignore_broken_paras = ignore_failing_paras;
        pp->register_unknown_fields = register_unknown_fields;
}

void para_init(para_parser_t * pp, para_t * para)
{
	para->common = pp;
	para->line = pp->line;
	para->start = 0;
	para->end = 0;
	para->fields = 0;
	para->nfields = 0;
	para->nfields = fieldtrie_count();
	para->fields = malloc(para->nfields * sizeof *para->fields);
	if (para->fields == 0) fatal_enomem(0);
}

void para_finalize(para_t * para)
{
	para->common = 0;
	para->start = 0;
	para->end = 0;
	if (para->fields != 0) {
		free(para->fields);
	}
	para->nfields = 0;
	para->fields = 0;
}

static struct field_data * register_field(para_t * para,
                                          char const * s, size_t slen)
{
        size_t inx = fieldtrie_insert_n(s, slen)->inx;
        if (inx >= para->nfields) {
                assert(para->nfields > 0);
                para->nfields = para->nfields * 2;
                para->fields = realloc(para->fields,
                                       para->nfields * sizeof(para->fields[0])
                                      );
                if (para->fields == 0) fatal_enomem(0);
        }
        struct field_data *field_data = &para->fields[inx];
        return field_data;
}

void para_parse_next(para_t * para)
{
redo:
	assert(para != 0);
	para_parser_t * pp = para->common;
	para->start = pp->loc;
	para->line = pp->line;
	for (size_t i = 0; i < para->nfields; i++) {
		para->fields[i].start = 0;
		para->fields[i].end = 0;
	}
	if (pp->invalidate_p) {
		fsaf_invalidate(pp->fp, para->start);
	}

	register size_t pos = para->start;
	register size_t line = para->line;
	register FSAF * fp = pp->fp;
	size_t field_start = 0;
	struct field_data * field_data = 0;

#define GETC (c = fsaf_getc(fp, pos++), c == '\n' ? line++ : line)
        int c;
START:
        GETC;
        switch (c) {
        case -1:
                pp->eof = true;
                goto END;
        case '\n':
                para->start++;
                goto START;
        default:
                field_start = --pos;
                goto FIELD_NAME;
        }
        assert(0);

FIELD_NAME:
        GETC;
        switch (c) {
        case '\n': case -1:
                if (pp->ignore_broken_paras) {
                        line_message(L_IMPORTANT,
                                     _("warning: expected a colon"),
                                     fp->fname,
                                     c == '\n' ?line-1:line);
                        goto FAIL;
                } else {
                        line_message(L_FATAL,
                                     _("expected a colon"),
                                     fp->fname,
                                     c == '\n' ?line-1:line);
                        fail();
                }
                break;
        case ':': {
                size_t len = (pos-1) - field_start;
                struct fsaf_read_rv r = fsaf_read(fp,
                                                  field_start,
                                                  len);
                assert(r.len == len);
                struct field_attr *attr =
                        fieldtrie_lookup(r.b, len);
                if (attr == NULL) {
                        if (para->common->
                            register_unknown_fields) {
                                field_data =
                                        register_field(para,
                                                       r.b,
                                                       len);
                        } else {
                                field_data = 0;
                        }
                } else {
                        assert(attr->inx < para->nfields);
                        field_data = &para->fields[attr->inx];
                }
                if (field_data != NULL) {
                        field_data->start = pos;
                        field_data->line = line;
                }
                goto BODY;
        }
        default:
                goto FIELD_NAME;
        }
        assert(0);

BODY:
        GETC;
        if (c == -1 || c == '\n') {
                if (field_data != 0) {
                        field_data->end = pos-1;
                        while (field_data->start < field_data->end
                               && fsaf_getc(fp, field_data->start)
                               == ' ') {
                                ++field_data->start;
                        }
                }
                goto BODY_NEWLINE;
        }
        if (c != -1) goto BODY; else goto BODY_NEWLINE;
        assert(0);

BODY_NEWLINE:
        GETC;
        switch (c) {
        case -1:
                //para->eof = true;
                /* pass through */
        case '\n':
                goto END;
        case ' ': case '\t':
                goto BODY_SKIPBLANKS;
        default:
                field_start = --pos;
		goto FIELD_NAME;
        }
        assert(0);

BODY_SKIPBLANKS:
        GETC;
        switch (c) {
        case -1:
                /* pass through */
        case '\n':
                goto END;
                break;
        case ' ': case '\t':
                goto BODY_SKIPBLANKS;
        default:
                goto BODY;
        }
        assert(0);

#undef GETC

FAIL:
        do {
                c = fsaf_getc(fp, pp->loc++);
                if (c == '\n') pp->line++;
        } while (c != -1 && c != '\n');
        goto redo;

END:
	para->end = pos-1;
	pp->loc = para->end;
	pp->line = fsaf_getc(fp, pp->loc) == '\n' ? line-1 : line;
}
