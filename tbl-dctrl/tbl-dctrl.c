/*  dctrl-tools - Debian control file inspection tools
    Copyright © 2005, 2006, 2007, 2008, 2009 Antti-Juhani Kaijanaho

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

#include <argp.h>
#include <locale.h>
#include <stdlib.h>
#include "misc.h"
#include "msg.h"
#include "i18n.h"
#include "ifile.h"
#include "para_bundle.h"
#include "sorter.h"

enum {
        OPT_SILENT = 256,
        OPT_MMAP
};

const char * argp_program_version = "tbl-dctrl (dctrl-tools) " VERSION;
const char * argp_program_bug_address = MAINTAINER;

static struct argp_option options[] = {
	{ "delimiter",         'd', N_("DELIM"),    0, N_("Specify a delimiter.") },
        { "no-heading",        'H', 0,              0, N_("Do not print a table heading") },
	{ "column",            'c', N_("SPEC"),     0, N_("Append the specified column.") },
	{ "copying",	       'C', 0,		    0, N_("Print out the copyright license.") },
	{ "errorlevel",	       'l', N_("LEVEL"),    0, N_("Set debugging level to LEVEL.") },
	{ "mmap",               OPT_MMAP, 0,        0, N_("Attempt mmapping input files") },
	{ 0 }
};

#define MAX_FNAMES 4096
#define MAX_COLUMNS 4096

struct arguments {
	char *delim; // NULL if not specced
	/* True iff some column lengths are unknown and need to be
	 * calculated. */
	_Bool need_preprocessing;
        /* True if no headings should be printed. */
        _Bool no_heading;
	/* Number of columns specified */
	size_t num_columns;
	/* Number of file names seen.  */
	size_t num_fnames;
	/* Columns specified on the command line. */
	struct column {
		char *heading;
		size_t field_inx;
		size_t column_width;
	} columns[MAX_COLUMNS];
	/* File names seen on the command line.  */
	struct ifile fname[MAX_FNAMES];
};

size_t linewrap(char **res, char const *orig, size_t orig_len,
		size_t col_width)
{
	assert(col_width > 0);

        // these are primarily manipulated by the INSERT macro below
	size_t max = 16;        // the capacity of rv
	char *rv = malloc(max); // the prospective return value
	size_t len = 0;         // length of the string in rv

#define INSERT(c) do { \
	if (len >= max) { \
		max *= 2; \
		rv = realloc(rv, max); \
		if (rv == NULL) fatal_enomem(NULL); \
	} \
	rv[len++] = (c); \
} while (0)

	size_t ll = 0;          // the length of the current line, so far

        // the following two record the location of the closest
        // breakpoint so far
	size_t bpo = 0;        // indexes orig
        size_t bpr = 0;        // indexes rv

	size_t num_lines = 1;

	mblen(NULL, 0);
	for (size_t i = 0; i < orig_len; /**/) {
                if (orig[i] == '\n') {
                        i++;
                        ll = 0;
                        INSERT('\n');
                        num_lines++;
                        continue;
                }
		if (ll == col_width) {
			if (rv[bpr] == '\n') {
				// no suitable breakpoint on this line,
				// break here
			} else {
				assert(bpr < len);
				len = bpr;
				assert(bpo < i);
				i = bpo + 1; 
			}
			ll = 0;
			INSERT('\n');
			num_lines++;
			continue;
		}
		if (orig[i] == ' ' || orig[i] == '\t') {
			// record this breakpoint
			bpo = i;
			bpr = len;
		}
		int n = mblen(orig + i, orig_len - i);
		if (n <= 0) break;
		for (size_t j = 0; j < n; j++) INSERT(orig[i+j]);
		i += n;
		ll++;
	}
	INSERT('\0');
	*res = rv;
	return num_lines;
#undef INSERT
}

void print_line(struct arguments *args,
		struct fsaf_read_rv const columns[/*args->num_columns*/])
{
	struct print_line_data {
		char *wrapped;
		size_t wrapped_len;
		size_t inx;
	} data[args->num_columns];
	size_t num_lines = 0;
	for (size_t i = 0; i < args->num_columns; i++) {
		size_t n = linewrap(&data[i].wrapped,
				    columns[i].b, columns[i].len,
				    args->columns[i].column_width);
		if (n > num_lines) num_lines = n;
		data[i].inx = 0;
		data[i].wrapped_len = strlen(data[i].wrapped);
	}
	for (size_t i = 0; i < num_lines; i++) {
		for (size_t j = 0; j < args->num_columns; j++) {
			if (args->delim == NULL) {
				fputs("| ", stdout);
			} else if (j > 0) {
				fputs(args->delim, stdout);
			}
			mblen(NULL, 0);
			size_t k;
			size_t m = 0;
			for (k = data[j].inx;
			     k < data[j].wrapped_len;
			     /**/) {
				if (data[j].wrapped[k] == '\n') {
					k++;
					break;
				}
				int n = mblen(data[j].wrapped + k,
					      data[j].wrapped_len - k);
				for (int l = 0; l < n; l++) {
					putchar(data[j].wrapped[k+l]);
				}
				if (n <= 0) n = 1;
				k += n;
				m++;
			}

			if (args->delim == NULL) {
				while (m < args->columns[j].column_width) {
					m++;
					putchar(' ');
				}
				putchar(' ');
			}
			data[j].inx = k;
		}
		if (args->delim == NULL) fputs("|", stdout);
		putchar('\n');
	}
	for (size_t i = 0; i < args->num_columns; i++) free(data[i].wrapped);
}

void print_head(struct arguments *args)
{
	if (args->delim == NULL) {
		for (size_t i = 0; i < args->num_columns; i++) {
			putchar('+');
			for (size_t j = 0; 
			     j < args->columns[i].column_width + 2;
			     j++) {
				putchar('=');
			}
		}
		putchar('+');
		putchar('\n');
	}
		
        if (!args->no_heading) {       
                struct fsaf_read_rv columns[args->num_columns];
                for (size_t i = 0; i < args->num_columns; i++) {
                        columns[i].b = args->columns[i].heading;
                        columns[i].len = strlen(columns[i].b);
                }
                print_line(args, columns);
                if (args->delim == NULL) {
                        for (size_t i = 0; i < args->num_columns; i++) {
                                putchar('+');
                                for (size_t j = 0;
                                     j < args->columns[i].column_width + 2;
                                     j++) {
                                        putchar('-');
                                }
                        }
                        putchar('+');
                        putchar('\n');
                }
        }
}

void print_foot(struct arguments *args)
{
	if (args->delim != NULL) return;
	for (size_t i = 0; i < args->num_columns; i++) {
		putchar('+');
		for (size_t j = 0; j < args->columns[i].column_width + 2; j++) {
			putchar('=');
		}
	}
	putchar('+');
	putchar('\n');
}

void handle_para(struct arguments *args, struct paragraph *para)
{
	struct fsaf_read_rv columns[args->num_columns];
	for (size_t i = 0; i < args->num_columns; i++) {	
		columns[i] = get_field(para, args->columns[i].field_inx, -1);
	}
	print_line(args, columns);
}

static error_t parse_opt (int key, char * arg, struct argp_state * state)
{
	struct arguments * args = state->input;
	switch (key) {
	case 'C':
		if (!to_stdout (COPYING)) fail();
		exit(0);
	case 'c': {
		char *carg = strdup(arg);
		if (carg == NULL) fatal_enomem(0);
		char *lens = strrchr(carg, ':');
		if (lens != NULL) *(lens++) = '\0'; else lens = "";
		char *fn = strchr(carg, '=');
		if (fn != NULL) *(fn++) = '\0';
		struct column *col = &args->columns[args->num_columns];
		col->heading = carg;
		col->field_inx = fieldtrie_insert(fn == NULL ? carg : fn)->inx;
		size_t n = 0;
		_Bool err = 0;
		for (char const *p = lens; *p != '\0'; p++) {
			if (!('0' <= *p && *p <= '9')) {
				err = 1;
				continue;
			}
			n = n * 10 + (*p - '0');
		}
		if (err) message(L_IMPORTANT, 0, _("invalid column length"));
		col->column_width = n > 0 ? n : -1;
		if (n == 0) args->need_preprocessing = 1;
		args->num_columns++;
	}
		break;
	case 'd':
		args->delim = strdup(arg);
		if (args->delim == NULL) fatal_enomem(NULL);
		break;
        case 'H':
                args->no_heading = 1;
                break;
	case 'l': {
		int ll = str2loglevel(arg);
		if (ll < 0)
		{
			message(L_FATAL, 0, _("no such log level '%s'"), arg);
			fail();
		}
		set_loglevel(ll);
		debug_message("parse_opt: l", 0);
	}
		break;
	case ARGP_KEY_ARG:
		debug_message("parse_opt: argument", 0);
		{
			char const * s;
			if (args->num_fnames >= MAX_FNAMES) {
				message(L_FATAL, 0, _("too many file names"));
				fail();
			}
			s = strdup(arg);
			if (s == 0) fatal_enomem(0);
			args->fname[args->num_fnames++] =
				(struct ifile){ .mode = m_read, .s = s };
		}
		break;
	case OPT_MMAP:
		debug_message("parse_opt: mmap", 0);
		fsaf_mmap = 1;
		break;
	case ARGP_KEY_END:
	case ARGP_KEY_ARGS:  case ARGP_KEY_INIT: case  ARGP_KEY_SUCCESS:
	case ARGP_KEY_ERROR: case ARGP_KEY_FINI: case ARGP_KEY_NO_ARGS:
		debug_message("parse_opt: ignored", 0);
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static char progdoc [] =
N_("tbl-dctrl -- tabularize Debian control files");

static struct argp argp = { options, parse_opt, 0, progdoc };

static size_t mbs_len(char const *mbs, size_t n, char const *fname)
{
	if (n == (size_t)(-1)) n = strlen(mbs);
        size_t mlen = 0;
	size_t len = 0;
	mblen(NULL, 0);
	for (size_t k = 0; k < n;/**/) {
		if (mbs[k] == '\n') {
                        if (len > mlen) mlen = len;
                        len = 0;
                        k++;
                        continue;
                }
		len++;
		int delta = mblen(mbs + k, n - k);
		if (delta <= 0) {
			message(L_IMPORTANT, fname, 
                                _("bad multibyte character"));
			k++;
		} else {
			k += delta;
		}
	}
	return len > mlen ? len : mlen;
}

int main(int argc, char * argv[])
{
	setlocale(LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	fieldtrie_init();

	static struct arguments args;

	//keys_init(&args.keys);
	msg_set_progname(argv[0]);
	argp_parse (&argp, argc, argv, ARGP_IN_ORDER, 0, &args);

	if (args.num_fnames == 0) {
		args.fname[args.num_fnames++] = (struct ifile){
			.mode = m_read, .s = "-"
		};
	}

	if (args.need_preprocessing) {
		struct para_bundle pb;

		bundle_init(&pb);
		
		for (size_t i = 0; i < args.num_fnames; i++) {
			bundle_slurp(&pb, args.fname[i]);
		}

		size_t n = fieldtrie_count();
		size_t lens[n];
		for (size_t i = 0; i < n; i++) lens[i] = 0;

		for (size_t i = 0; i < pb.num_paras; i++) {
			struct paragraph *p = pb.paras[i];
			assert(p->nfields == n);
			for (size_t j = 0; j < p->nfields; j++) {
				struct fsaf_read_rv r = get_field(p, j, -1);
				size_t len = mbs_len(r.b, r.len,
						     p->common->fp->fname);
				if (len > lens[j]) lens[j] = len;
			}
		}
		for (size_t i = 0; i < args.num_columns; i++) {
			size_t width = args.columns[i].column_width;
			size_t headw = mbs_len(args.columns[i].heading, -1, 0);
			if (width == (size_t)(-1)) width = lens[i];
			if (headw > width) width = headw;
			args.columns[i].column_width = width;
		}
		print_head(&args);
		for (size_t i = 0; i < pb.num_paras; i++) {
			handle_para(&args, pb.paras[i]);
		}
		print_foot(&args);
	} else {
		print_head(&args);
		for (size_t i = 0; i < args.num_fnames; ++i) {
			int fd;
			struct ifile fname = args.fname[i];
			
			if (fname.mode == m_error) continue;
			
			fd = open_ifile(fname);
			if (fd == -1) break;
			
			if (!chk_ifile(fname, fd)) break;

			FSAF * fp = fsaf_fdopen(fd, fname.s);
			para_parser_t pp;
			para_parser_init(&pp, fp, true, false, false);
			para_t para;
			para_init(&pp, &para);
			while (1) {
				para_parse_next(&para);
				if (para_eof(&pp)) break;
				handle_para(&args, &para);
			}
			
			close_ifile(fname, fd);
		}
		print_foot(&args);
	}

	return 0;
}
