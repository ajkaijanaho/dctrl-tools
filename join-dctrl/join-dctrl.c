/*  dctrl-tools - Debian control file inspection tools
    Copyright © 2007 Antti-Juhani Kaijanaho

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
#include <assert.h>
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include "misc.h"
#include "msg.h"
#include "i18n.h"
#include "ifile.h"
#include "para_bundle.h"

enum {
        OPT_SILENT = 256,
        OPT_MMAP
};

const char * argp_program_version = "join-dctrl (dctrl-tools) " VERSION;
const char * argp_program_bug_address = MAINTAINER;

static struct argp_option options[] = {
        { "1st-join-field",    '1', N_("FIELD"),    0, N_("Specify the join field to use for the first file") },
        { "2nd-join-field",    '2', N_("FIELD"),    0, N_("Specify the join field to use for the second file") },
        { "join-field",        'j', N_("FIELD"),    0, N_("Specify the common join field") },
        { "output-fields",     'o', N_("FIELDSPEC"),0, N_("Specify the format of the output file") },
	{ "copying",	       'C', 0,		    0, N_("Print out the copyright license.") },
	{ "errorlevel",	       'l', N_("LEVEL"),    0, N_("Set debugging level to LEVEL.") },
	{ "mmap",               OPT_MMAP, 0,        0, N_("Attempt mmapping input files") },
	{ 0 }
};

#define MAX_FNAMES 2

struct arguments {
        struct field_attr *join_field[MAX_FNAMES];
        size_t num_fnames;
        struct ifile fname[MAX_FNAMES];
        size_t num_show_fields;
        struct show_field {
                int file_inx; /* indexes join_field, can be -1 to indicate
                                 common (ignoring nulls) join field
                               */
                struct field_attr *field; // null if file_inx is -1
                char *showname;
        } show_fields[MAX_FIELDS];
};

static error_t parse_opt (int key, char * arg, struct argp_state * state)
{
	struct arguments * args = state->input;
	switch (key) {
        case '1': case '2': case 'j': {
                static const char the_other_key[] = { '2', '1' };
                static const char *errmsg[] = {
                        N_("the join field of the first file has already been specified"),
                        N_("the join field of the second file has already been specified")
                };
                for (size_t i = 0; i < 2; i++) {
                        if (key == the_other_key[i]) continue;
                        if (args->join_field[0] != NULL) {
                                message(L_FATAL, gettext(errmsg[i]), 0);
                                fail();
                        }
                }
                for (size_t i = 0; i < 2; i++) {
                        if (key == the_other_key[i]) continue;
                        char *str = strdup(arg);
                        if (str == NULL) fatal_enomem(0);
                        args->join_field[i] = fieldtrie_insert(str);
                }
                
        }
                break;
        case 'o': {
                char * carg = strdup(arg);
                if (carg == 0) fatal_enomem(0);
                for (char * s = strtok(carg, ","); s != 0; s = strtok(0, ",")){
                        if (args->num_show_fields >= MAX_FIELDS) {
                                message(L_FATAL, _("too many output fields"), 
                                        0);
                                fail();
                        }
                        struct show_field *of =
                                &args->show_fields[args->num_show_fields++];

                        if (s[0] == '0' && s[1] == '\0') {
                                of->file_inx = -1;
                                of->field = NULL;
                                of->showname = NULL;
                                continue;
                        }
                        char * fld = strchr(s, '.');
                        if (fld == NULL) {
                                message(L_FATAL, _("missing '.' in output "
                                                   "field specification"), 0);
                                fail();
                        }
                        *fld = '\0';
                        ++fld;
                        char * shw = strchr(fld, ':');
                        if (shw != NULL) {
                                *shw = '\0';
                                ++shw;
                        }
                        int file_inx = 0;
                        if (s[0] == '1' && s[1] == '\0') {
                                file_inx = 0;
                        } else if (s[0] == '2' && s[1] == '\0') {
                                file_inx = 1;
                        } else {
                                message(L_FATAL, _("expected either '1.' or "
                                                   "'2.' at the start of the "
                                                   "field specification"), 
                                        0);
                                        fail();
                        }
                        of->file_inx = file_inx;
                        of->field = fieldtrie_insert(fld);
                        of->showname = shw == NULL ? NULL : strdup(shw);
                        if (shw != NULL && of->showname == NULL) {
                                fatal_enomem(0);
                        }
                }
                free(carg);
        }
                break;
	case 'C':
		if (!to_stdout (COPYING)) fail();
		exit(0);
	case 'l': {
		int ll = str2loglevel(arg);
		if (ll < 0)
		{
			message(L_FATAL, _("no such log level"), arg);
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
				message(L_FATAL, _("too many file names"), 0);
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

static char progdoc [] = N_("join-dctrl -- join two Debian control files");

static struct argp argp = { options, parse_opt, 0, progdoc };

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

        if (args.num_fnames != 2) {
                message(L_FATAL, _("need exactly two input files"), 0);
                fail();
        }

        int fd[MAX_FNAMES];
        FSAF *fp[MAX_FNAMES];
        para_parser_t pp[MAX_FNAMES];
        para_t para[MAX_FNAMES];
        
        for (size_t i = 0; i < args.num_fnames; i++) {
                fd[i] = open_ifile(args.fname[i]);
                if (fd[i] == -1) fail();
                if (!chk_ifile(args.fname[i], fd[i])) fail();
                fp[i] = fsaf_fdopen(fd[i], args.fname[i].s);
                
                para_parser_init(&pp[i], fp[i], true, false,
                                 args.num_show_fields == 0);
                para_init(&pp[i], &para[i]);
                para_parse_next(&para[i]);
        }

        for (size_t i = 0; i < args.num_fnames; i++) {
                for (size_t j = i+1; j < args.num_fnames; j++) {
                        if (fd[i] == fd[j]) {
                                message(L_FATAL, 
                                        _("cannot join a stream with itself"),
                                        0);
                                fail();
                        }
                } 
        }

        while (true) {
                struct fsaf_read_rv a;
        again:
                a = get_field(&para[0], args.join_field[0]->inx, -1);
                for (size_t i = 1; i < args.num_fnames; i++) {
                        if (para_eof(&pp[i]) || para_eof(&pp[i-1])) goto done;
                        struct fsaf_read_rv b =
                                get_field(&para[i],
                                          args.join_field[i]->inx, -1);
#ifdef INCLUDE_DEBUG_MSGS
                        if (do_msg(L_DEBUG)) {
                                fprintf(stderr, "join-cmp %.*s %.*s => ",
                                        (int)a.len, a.b, (int)b.len, b.b);
                        }
#endif
                        size_t len = a.len < b.len ? a.len : b.len;
                        int r = strncmp(a.b,b.b,len);
                        if (r == 0) {
                                if (a.len < b.len) r = -1;
                                if (a.len > b.len) r = 1;
                        }
#ifdef INCLUDE_DEBUG_MSGS
                        if (do_msg(L_DEBUG)) {
                                fprintf(stderr, "%d\n", r);
                        }
#endif
                        if (r < 0) {
                                para_parse_next(&para[i-1]);
                                goto again;
                        }
                        if (r > 0) {
                                para_parse_next(&para[i]);
                                goto again;
                        }
                        a = b;
                }
#ifdef INCLUDE_DEBUG_MSGS
                        if (do_msg(L_DEBUG)) {
                                fprintf(stderr, "join-show!\n");
                        }
#endif
                /* now all paras should be in a join configuration;
                   let's print out the result */
                if (args.num_show_fields == 0) {
                        assert(false); // unimplemented as of yet
                        continue;
                }
                for (size_t i = 0; i < args.num_show_fields; i++) {
                        struct show_field *sf = &args.show_fields[i];
                        struct fsaf_read_rv body = { .b = "", .len = 0 };
                        const char *showname;
                        if (sf->file_inx == -1) {
                                for (size_t j = 0;
                                     body.len == 0 && j < args.num_fnames;
                                     j++) {
                                        body = get_field(&para[j],
                                                         args.join_field[j]->inx,
                                                         -1);
                                        showname = args.join_field[j]->name;
                                }
                        } else {
                                body = get_field(&para[sf->file_inx],
                                                 sf->field->inx,
                                                 -1);
                                showname = sf->showname == NULL
                                        ? sf->field->name
                                        : sf->showname;
                        }
                        fputs(showname, stdout);
                        fputs(": ", stdout);
                        fwrite(body.b, 1, body.len, stdout);
                        putchar('\n');
                }
                putchar('\n');
                para_parse_next(&para[0]);
        }
done:

	return errors_reported() ? 2 : 0;
}

#if MAX_FNAMES != 2
// for example, there are command line options for handling only two files
#error "it's not enough to just edit the #define of MAX_FNAMES :)"
#endif
