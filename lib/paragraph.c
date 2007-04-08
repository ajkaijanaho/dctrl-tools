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
		      bool invalidate_p, bool ignore_failing_paras)
{
	pp->fp = fp;
	pp->eof = false;
	pp->loc = 0;
	pp->line = 1;
	pp->invalidate_p = invalidate_p;
	pp->ignore_broken_paras = ignore_failing_paras;
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

void para_parse_next(para_t * para)
{
redo:
	debug_message("para_parse_next", 0);
	assert(para != 0);
	para_parser_t * pp = para->common;
	assert(para->nfields == fieldtrie_count());
	para->start = pp->loc;
	para->line = pp->line;
	for (size_t i = 0; i < para->nfields; i++) {
		para->fields[i].start = 0;
		para->fields[i].end = 0;
	}
	if (pp->invalidate_p) {
		fsaf_invalidate(pp->fp, para->start);
	}
	register enum { START, FIELD_NAME, BODY, BODY_NEWLINE,
			BODY_SKIPBLANKS, END, FAIL } state = START;
	register size_t pos = para->start;
	register size_t line = para->line;
	register FSAF * fp = pp->fp;
	size_t field_start = 0;
	struct field_data * field_data = 0;
	while (state != END && state != FAIL) {
#               ifndef TEST_NODEBUG
		static char * const stnm[] = { "START", "FIELD_NAME",
					       "BODY", "BODY_NEWLINE",
					       "BODY_SKIPBLANKS", "END",
					       "FAIL" };
		if (do_msg(L_DEBUG)) {
			fprintf(stderr, "%s:%zu: state: %s\n",
				fp->fname, line, stnm[state]);
		}
#               endif
		int c = fsaf_getc(fp, pos++);
		if (c == '\n') line++;
		switch (state) {
		case START:
			switch (c) {
			case -1:
				pp->eof = true;
				state = END;
				break;
			case '\n':
				para->start++;
				break;
			default:
				field_start = --pos;
				state = FIELD_NAME;
			}
			break;
		case FIELD_NAME:
			switch (c) {
			case '\n': case -1:
				if (pp->ignore_broken_paras) {
					line_message(L_IMPORTANT,
						     _("warning: "
						       "expected a colon"),
						     fp->fname,
                                                     c == '\n' ?line-1:line);
					state = FAIL;
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
				struct field_attr attr =
					fieldtrie_lookup(r.b, len);
				if (!attr.valid) {
					field_data = 0;
				} else {
					assert(attr.inx < para->nfields);
					field_data = &para->fields[attr.inx];
					field_data->start = pos;
					field_data->line = line;
				}
				state = BODY;
			}
				break;
			}
			break;
		case BODY:
			if (c == -1 || c == '\n') {
				if (field_data != 0) {
					field_data->end = pos-1;
					while (field_data->start < field_data->end
					       && fsaf_getc(fp, field_data->start)
					       == ' ') {
						++field_data->start;
					}
				}
				state = BODY_NEWLINE;
			}
			if (c != -1) break;
			/* conditional passthrough */
		case BODY_NEWLINE:
			switch (c) {
			case -1:
				//para->eof = true;
				/* pass through */
			case '\n':
				state = END;
				break;
			case ' ': case '\t':
				state = BODY_SKIPBLANKS;
				break;
			default:
				field_start = --pos;
				state = FIELD_NAME;
			}
			break;
		case BODY_SKIPBLANKS:
			switch (c) {
			case -1:
				/* pass through */
			case '\n':
				state = END;
				break;
			case ' ': case '\t':
				break;
			default:
				state = BODY;
			}
			break;
		default: assert(0);
		}
	}
	para->end = pos-1;
	pp->loc = para->end;
	pp->line = fsaf_getc(fp, pp->loc) == '\n' ? line-1 : line;

	if (state == FAIL) {
		/* skip the rest of the broken line */
		int c;
		do {
			c = fsaf_getc(fp, pp->loc++);
			if (c == '\n') pp->line++;
		} while (c != -1 && c != '\n');
		goto redo;
	}
}
