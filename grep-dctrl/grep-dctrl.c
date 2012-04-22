/*  dctrl-tools - Debian control file inspection tools
    Copyright © 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
                2010, 2011, 2012
                Antti-Juhani Kaijanaho

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

#include <argp.h>
#include <assert.h>
#include <fcntl.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "atom.h"
#include "fnutil.h"
#include "fsaf.h"
#include "i18n.h"
#include "ifile.h"
#include "misc.h"
#include "msg.h"
#include "paragraph.h"
#include "predicate.h"
#include "strlist.h"
#include "util.h"

const char * argp_program_version = "grep-dctrl (dctrl-tools) " VERSION;
const char * argp_program_bug_address = MAINTAINER;

const char description [] = "Description";
struct field_attr *description_attr;

static char progdoc [] = N_("grep-dctrl -- grep Debian control files");

static char argsdoc [] = N_("FILTER [FILENAME...]");

enum {
        OPT_OPTPARSE=256,
        OPT_SILENT,
        OPT_EQ,
        OPT_LT,
        OPT_LE,
        OPT_GT,
        OPT_GE,
        OPT_MMAP,
        OPT_IGN_ERRS,
        OPT_ENSURE,
        OPT_COMPAT,
        OPT_PATTERN
};

static struct argp_option options[] = {
	{ "errorlevel",	    'l', N_("LEVEL"),	    0, N_("Set log level to LEVEL.") },
	{ "field",	    'F', N_("FIELD,FIELD,..."), 0, N_("Restrict pattern matching to the FIELDs given.") },
	{ 0,		    'P', 0,		    0, N_("This is a shorthand for -FPackage.") },
	{ 0,		    'S', 0,		    0, N_("This is a shorthand for -FSource:Package.") },
	{ "show-field",	    's', N_("FIELD,FIELD,..."), 0, N_("Show only the body of these fields from the matching paragraphs.") },
	{ 0,		    'd', 0,		    0, N_("Show only the first line of the \"Description\" field from the matching paragraphs.") },
	{ "no-field-names", 'n', 0,		    0, N_("Suppress field names when showing specified fields.") },
	{ "eregex",	    'e', 0,		    0, N_("Regard the pattern as an extended POSIX regular expression.") },
	{ "regex",	    'r', 0,		    0, N_("Regard the pattern as a standard POSIX regular expression.") },
	{ "ignore-case",    'i', 0,		    0, N_("Ignore case when looking for a match.") },
	{ "invert-match",   'v', 0,		    0, N_("Show only paragraphs that do not match.") },
        { "invert-show",    'I', 0,                 0, N_("Show those fields that have NOT been selected with -s") },
	{ "count",	    'c', 0,		    0, N_("Show only the count of matching paragraphs.") },
	{ "exact-match",    'X', 0,		    0, N_("Do an exact match.") },
	{ "copying",	    'C', 0,		    0, N_("Print out the copyright license.") },
	{ "and",	    'a', 0,		    0, N_("Conjunct filters.") },
	{ "or",		    'o', 0,		    0, N_("Disjunct filters.") },
	{ "not",	    '!', 0,		    0, N_("Negate the following filters.") },
	{ "eq",		    OPT_EQ, 0,		    0, N_("Test for version number equality.") },
	{ "lt",		    OPT_LT, 0,		    0, N_("Version number comparison: <<.") },
	{ "le",		    OPT_LE, 0,		    0, N_("Version number comparison: <=.") },
	{ "gt",		    OPT_GT, 0,		    0, N_("Version number comparison: >>.") },
	{ "ge",		    OPT_GE, 0,		    0, N_("Version number comparison: >=.") },
	{ "debug-optparse", OPT_OPTPARSE, 0,	    0, N_("Debug option parsing.") },
	{ "quiet",	    'q', 0,		    0, N_("Do not output to stdout.") },
	{ "silent",	    OPT_SILENT, 0,	    0, N_("Do not output to stdout.") },
	{ "mmap",           OPT_MMAP, 0,            0, N_("Attempt mmapping input files") },
	{ "ignore-parse-errors", OPT_IGN_ERRS, 0,   0, N_("Ignore parse errors") },
        { "pattern",        OPT_PATTERN, N_("PATTERN"), 0, N_("Specify the pattern to search for") },
	{ "whole-pkg",	    'w', 0,                 0, N_("Match only whole package names (this implies -e)") },
        { "ensure-dctrl",   OPT_ENSURE, 0,          0, N_("Ensure that the output is in dctrl format (overridden by -n)") },
        { "compat",         OPT_COMPAT, 0,          0, N_("Override the effect of an earlier --ensure-dctrl") },
	{ 0 }
};


// Tokens
#define TOK_EOD    0
#define TOK_NOT    1
#define TOK_AND    2
#define TOK_OR     3
#define TOK_LP     4 /* left paren */
#define TOK_RP     5 /* right paren */
#define TOK_EXACT  6 /* -X */
#define TOK_ERGEX  7 /* -e */
#define TOK_REGEX  8 /* -r */
#define TOK_EQ     9
#define TOK_LT    10
#define TOK_LE    11
#define TOK_GT    12
#define TOK_GE    13
#define TOK_ICASE 14 /* -i */
#define TOK_PAT   15 /* --pattern */
#define TOK_STR   16 /* a plain string */
#define TOK_WHOLE 17 /* --whole-pkg */
#define TOK_FIELD 18 /* -F */

#define MAX_FNAMES 4096
#define MAX_TOKS   16384

static int debug_optparse = 0;

struct arguments {
	/* Parser state flag: last token seen was ')' */
	bool just_seen_cparen;
	/* Number of file names seen.  */
	size_t num_fnames;
	/**/
	size_t num_show_fields;
	/* A machine-readable representation of the predicate.  */
	struct predicate * p;
	/* Ignore parse errors? */
	bool ignore_errors;
	/* Quiet operation? */
	bool quiet;
	/* Do show field names? */
	bool show_field_name;
        /* Ensure that the output is in dctrl format? (Ignored if
           show_field_name is false.) */
        bool ensure_dctrl;
	/* Do show (only) first line of Description? */
	bool short_descr;
	/* Does show_fields contain Description? */
	bool description_selected;
	/* Count matching paragraphs? */
	bool count;
	/* Invert match? */
	bool invert_match;
        /* Show fields that are NOT listed? */
        bool invert_show;
	/* First unused position in toks.  */
	size_t toks_np;
        /* Token read position. */
	size_t toks_pos;
	/* Token stream for the predicate parser. */
	int toks[MAX_TOKS];
        /* The string value, if any, of each token*/
        char * strings[MAX_TOKS];
	/* File names seen on the command line.  */
	struct ifile fname[MAX_FNAMES];
        /**/
        size_t show_fields[MAX_FIELDS];
};

#define IS_SHOW_FIELD(field_app_data) ((field_app_data) & 1)
#define SET_SHOW_FIELD(field_app_data,val) \
  ((field_app_data) = ((field_app_data & ~1) | val))
#define GET_BACKUP_FIELD(field_app_data) (((field_app_data & ~0)) == (unsigned)-1 ? (size_t)-1 : (size_t)(field_app_data) >> 1)
#define SET_BACKUP_FIELD(field_app_data,val) \
  ((field_app_data) = (((field_app_data)&1) | (val<<1)))

#define APPTOK(tok) do { apptok(args, (tok)); } while (0)

static void apptok(struct arguments * args, const int tok)
{
	debug_message("apptok", 0);
	if (args->toks_np >= MAX_TOKS) {
		message(L_FATAL, 0, _("filter is too long"));
		fail();
	}
	args->toks[args->toks_np++] = tok;
        args->strings[args->toks_np-1] = 0;
}

#define APPSTR(tok,str) do { appstr(args, (tok), (str)); } while (0)

static void appstr(struct arguments * args, const int tok, char * str) {
        debug_message("appstr", 0);
        apptok(args, tok);
        args->strings[args->toks_np-1] = str;
}

static error_t parse_opt (int key, char * arg, struct argp_state * state)
{
	struct arguments * args = state->input;
	debug_message("parse_opt", 0);
	switch (key) {
                char *carg;
	case 'C':
		if (!to_stdout (COPYING)) fail();
		exit(0);
        case OPT_ENSURE:
                args->ensure_dctrl = true;
                break;
        case OPT_COMPAT:
                args->ensure_dctrl = false;
                break;
	case 'v':
		args->invert_match = true;
		break;
	case 'c':
		args->count = true;
		break;
	case 'q': case OPT_SILENT:
		debug_message("parse_opt: q", 0);
		args->quiet = true;
		break;
	case 'n':
		debug_message("parse_opt: n", 0);
		args->show_field_name = false;
		break;
	case 'd':
		args->short_descr = true;
		break;
        case 'I':
                args->invert_show = true;
                break;
	case 's': {
		char * carg = strdup(arg);
		if (carg == 0) fatal_enomem(0);
		for (char * s = strtok(carg, ","); s != 0; s = strtok(0, ",")){
                        char * repl = strchr(s, ':');
                        if (repl != NULL) {
                                *repl = '\0';
                                ++repl;
                        }
                        struct field_attr *fa = fieldtrie_insert(s);
                        if (args->num_show_fields >= MAX_FIELDS) {
                                message(L_FATAL, 0, _("too many output fields"));
                                fail();
                        }
                        args->show_fields[args->num_show_fields] = fa->inx;
                        if (fa == description_attr) {
                                args->description_selected = true;
                        }
                        SET_SHOW_FIELD(fa->application_data, true);

                        size_t repl_inx = repl == NULL
                                ? (size_t)(-1)
                                : fieldtrie_insert(repl)->inx;

                        SET_BACKUP_FIELD(fa->application_data, repl_inx);

			++args->num_show_fields;
		}
		free(carg);
	}
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
	case '!':
		debug_message("parse_opt: !", 0);
		APPTOK(TOK_NOT);
		break;
	case 'a':
		debug_message("parse_opt: a", 0);
		APPTOK(TOK_AND);
		break;
	case 'o':
		debug_message("parse_opt: o", 0);
		APPTOK(TOK_OR);
		break;
        case 'S':
                debug_message("parse_opt: S", 0);
                arg = "Source:Package";
                goto case_F;
	case 'P':
		debug_message("parse_opt: P", 0);
		arg = "Package";
                goto case_F;
        case_F:
	case 'F':
		debug_message("parse_opt: Fv", 0);
		carg = strdup(arg);
		if (carg == 0) fatal_enomem(0);
		for (char * s = strtok(carg, ","); s != 0; s = strtok(0, ",")){
			char * tmp = strdup(s);
			if (tmp == 0) fatal_enomem(0);
                        APPSTR(TOK_FIELD, tmp);
		}
		free(carg);
		break;
	case 'X':
		debug_message("parse_opt: X", 0);
		APPTOK(TOK_EXACT);
		break;
	case 'r':
		debug_message("parse_opt: r", 0);
		APPTOK(TOK_REGEX);
		break;
	case 'e':
		debug_message("parse_opt: e", 0);
		APPTOK(TOK_ERGEX);
		break;
	case OPT_EQ:
		debug_message("parse_opt: eq", 0);
		APPTOK(TOK_EQ);
		break;
	case OPT_LT:
		debug_message("parse_opt: lt", 0);
		APPTOK(TOK_LT);
		break;
	case OPT_LE:
		debug_message("parse_opt: le", 0);
		APPTOK(TOK_LE);
		break;
	case OPT_GT:
		debug_message("parse_opt: gt", 0);
		APPTOK(TOK_GT);
		break;
	case OPT_GE:
		debug_message("parse_opt: ge", 0);
		APPTOK(TOK_GE);
		break;
	case OPT_MMAP:
		debug_message("parse_opt: mmap", 0);
		fsaf_mmap = 1;
		break;
	case 'i':
		debug_message("parse_opt: i", 0);
                APPTOK(TOK_ICASE);
		break;
	case OPT_OPTPARSE:
		debug_message("parse_opt: optparse", 0);
		debug_optparse = 1;
		break;
	case OPT_IGN_ERRS:
		debug_message("parse_opt: ignore-parse-errors", 0);
		args->ignore_errors = 1;
		break;
        case OPT_PATTERN:
                debug_message("parse_opt: pattern", 0);
		carg = strdup(arg);
		if (carg == 0) fatal_enomem(0);
                APPSTR(TOK_PAT, carg);
		break;
	case 'w':
		debug_message("parse_opt: whole-pkg", 0);
                APPTOK(TOK_WHOLE);
		break;
	case ARGP_KEY_ARG:
		debug_message("parse_opt: argument", 0);
		if (strcmp(arg, "!") == 0) {
			debug_message("parse_opt: !", 0);
			APPTOK(TOK_NOT);
			break;
		}
		if (strcmp(arg, "(") == 0) {
			debug_message("parse_opt: (", 0);
			APPTOK(TOK_LP);
			break;
		}
		if (strcmp(arg, ")") == 0) {
			debug_message("parse_opt: )", 0);
			APPTOK(TOK_RP);
			break;
		}
                carg = strdup(arg);
		if (carg == 0) fatal_enomem(0);
                APPSTR(TOK_STR, carg);
		break;
	case ARGP_KEY_END:
		debug_message("parse_opt: end", 0);
		break;
	case ARGP_KEY_ARGS:  case ARGP_KEY_INIT: case  ARGP_KEY_SUCCESS:
	case ARGP_KEY_ERROR: case ARGP_KEY_FINI: case ARGP_KEY_NO_ARGS:
		debug_message("parse_opt: ignored", 0);
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static void dump_args(struct arguments * args)
{
        predicate_print(args->p);
	printf("num_fnames = %zi\n", args->num_fnames);
	for (size_t i = 0; i < args->num_fnames; i++) {
		printf("fname[%zi].mode = %s, fname[%zi].s = %s\n",
		       i, ifile_modes[args->fname[i].mode],
		       i, args->fname[i].s);
	}
}

static
int peek_token(struct arguments const * args)
{
	assert(args->toks_pos <= args->toks_np);
	if (args->toks_pos == args->toks_np) return TOK_EOD;
	return args->toks[args->toks_pos];
}

static
int get_token(struct arguments * args)
{
	assert(args->toks_pos <= args->toks_np);
	if (args->toks_pos == args->toks_np) return TOK_EOD;
	return args->toks[args->toks_pos++];
}

static
char * get_string(struct arguments * args)
{
	assert(args->toks_pos < args->toks_np);
	return args->strings[args->toks_pos++];
}

static const char *tokdescr(int tok) {
	switch (tok) {
        case TOK_EOD:
                return "EOD";
	case TOK_NOT: 
		return "!";
	case TOK_AND:
		return "-a";
	case TOK_OR : 
		return "-o";
	case TOK_LP :
		return "(";
	case TOK_RP :
		return ")";
        case TOK_EXACT :
                return "-X";
        case TOK_ERGEX :
                return "-e";
        case TOK_REGEX :
                return "-r";
        case TOK_EQ :
                return "--eq";
        case TOK_LT :
                return "--lt";
        case TOK_LE :
                return "--le";
        case TOK_GT :
                return "--gt";
        case TOK_GE :
                return "--ge";
        case TOK_ICASE :
                return "-i";
        case TOK_PAT :
                return "--pattern";
        case TOK_STR :
                return "string";
        case TOK_WHOLE :
                return "-w";
        case TOK_FIELD :
                return "-F";
	default:
		message(L_FATAL, 0,
                        _("internal error: unknown token %d"), tok);
		fail();
	}
}

static void unexpected(int tok)
{
	switch (tok) {
	case TOK_EOD:
		message(L_FATAL, 0, _("unexpected end of filter"));
		fail();
        case TOK_PAT :
                message(L_FATAL, 0, _("unexpected pattern in command line"));
                fail();
        case TOK_STR :
                message(L_FATAL, 0, _("unexpected string in command line"));
                fail();
        default:
                message(L_FATAL, 0,
                        _("unexpected '%s' in command line"),
                        tokdescr(tok));
                fail();
	}
}

struct predicate_qualifiers {
        struct strlist *fields;
        enum matching_mode mm;
        bool ignore_case;
        bool whole_pkg;
};

static struct predicate * parse_conj(struct arguments * args,
                                     struct predicate_qualifiers pq);

/* prim -> primtok
   prim -> primtok prim
   prim -> TOK_LP conj TOK_RP
   prim -> TOK_PAT prim'
   prim -> TOK_STR prim'

   prim' ->
   prim' -> primtok prim'

   primtok -> TOK_FIELD
   primtok -> TOK_ERGEX | TOK_REGEX
   primtok -> TOK_ICASE | TOK_EXACT | TOK_WHOLE
   primtok -> TOK_EQ | TOK_LT | TOK_LE | TOK_GE | TOK_GT
*/
static struct predicate * parse_prim(struct arguments * args,
                                     struct predicate_qualifiers pq_)
{
        char *pattern = 0;
        struct predicate *rv = 0;

        struct predicate_qualifiers pq = pq_;
        struct strlist_memento mem = strlist_save(pq.fields);

        bool nonempty = false;
        while (1) {
                debug("tok = %s, mm = %d", tokdescr(peek_token(args)), pq.mm);
                switch (peek_token(args)) {
                case TOK_FIELD:
                        if (!strlist_append(pq.fields, get_string(args))) {
                                enomem(0);
                        }
                        break;
                case TOK_ERGEX:
                        if (pq.mm != M_SUBSTR) goto failmode;
                        pq.mm = M_EREGEX;
                        get_token(args);
                        break;
                case TOK_REGEX:
                        if (pq.mm != M_SUBSTR) goto failmode;
                        pq.mm = M_REGEX;
                        get_token(args);
                        break;
                case TOK_ICASE:
                        pq.ignore_case = true;
                        get_token(args);
                        break;
                case TOK_EXACT:
                        if (pq.mm != M_SUBSTR) goto failmode;
                        pq.mm = M_EXACT;
                        get_token(args);
                        break;
                case TOK_WHOLE:
                        if (pq.mm != M_SUBSTR) goto failmode;
                        pq.mm = M_EREGEX;
                        pq.whole_pkg = true;
                        get_token(args);
                        break;
                case TOK_EQ:
                        if (pq.mm != M_SUBSTR) goto failmode;
                        pq.mm = M_VER_EQ;
                        get_token(args);
                        break;
                case TOK_LT:
                        if (pq.mm != M_SUBSTR) goto failmode;
                        pq.mm = M_VER_LT;
                        get_token(args);
                        break;
                case TOK_LE:
                        if (pq.mm != M_SUBSTR) goto failmode;
                        pq.mm = M_VER_LE;
                        get_token(args);
                        break;
                case TOK_GT:
                        if (pq.mm != M_SUBSTR) goto failmode;
                        pq.mm = M_VER_GT;
                        get_token(args);
                        break;
                case TOK_GE:
                        if (pq.mm != M_SUBSTR) goto failmode;
                        pq.mm = M_VER_GE;
                        get_token(args);
                        break;
                case TOK_LP:
                {
                        get_token(args);
                        rv = parse_conj(args, pq);
                        if (get_token(args) != TOK_RP) {
                                message(L_FATAL, 0,
                                        _("missing ')' in command line"));
                                fail();
                        }
                        goto finally;
                }
                case TOK_PAT:
                        if (pattern != 0) {
                                message(L_FATAL, 0,
                                        _("Multiple patterns for the same "
                                          "simple filter are not allowed"));
                                fail();
                        }
                        /* passthrough */
                case TOK_STR:
                        if (pattern != 0) goto loop_done;
                        pattern = get_string(args);
                        break;
                default:
                        goto loop_done;
                }
                nonempty = true;
        } loop_done:

        if (!nonempty) {
                unexpected(get_token(args));
        }
        
        if (pattern == 0) {
                message(L_FATAL, 0, _("A pattern is mandatory"));
                fail();
        }
        
        if (strlist_is_empty(pq.fields)) {
                strlist_append(pq.fields, 0);
         }
        
        for (struct strlist_iterator it = strlist_begin(pq.fields);
             !strlist_iterator_at_end(it); strlist_iterator_next(&it)) {
                struct atom * atom = malloc(sizeof *atom);
                if (atom == 0) enomem(0);
                atom->field_name = strlist_iterator_get(it);
                atom->field_inx = -1;
                atom->mode = pq.mm;
                atom->ignore_case = pq.ignore_case;
                atom->whole_pkg = pq.whole_pkg;
                atom->pat = pattern;
                atom->patlen = strlen(pattern);
                atom_finish(atom);
                struct predicate *tmp = predicate_ATOM(atom);
                rv = rv != 0 ? predicate_OR(rv, tmp) : tmp;
        }
        
finally:
        strlist_restore(mem);
        return rv;
failmode:
        message(L_FATAL, 0, _("inconsistent modifiers of simple filters")); 
        fail();
        rv = 0;
        goto finally;
}

/* neg -> TOK_NOT prim
   neg -> prim
*/
static struct predicate * parse_neg(struct arguments * args,
                                    struct predicate_qualifiers pq)
{
	bool neg = false;
	if (peek_token(args) == TOK_NOT) {
		neg = true;
		get_token(args);
	}
	struct predicate * rv = parse_prim(args, pq);
	if (neg) rv = predicate_NOT(rv);
        return rv;
}

/* disj -> neg
   disj -> disj TOK_OR neg
*/
static struct predicate * parse_disj(struct arguments * args,
                                     struct predicate_qualifiers pq)
{
	struct predicate * rv = parse_neg(args, pq);
	while (peek_token(args) == TOK_OR) {
		get_token(args);
		struct predicate * tmp = parse_neg(args, pq);
                rv = predicate_OR(rv, tmp);
	}
        return rv;
}

/* conj -> disj
   conj -> conj TOK_AND disj
*/
static struct predicate * parse_conj(struct arguments * args,
                                     struct predicate_qualifiers pq)
{
	struct predicate * rv = parse_disj(args, pq);
	while (peek_token(args) == TOK_AND) {
		get_token(args);
		struct predicate * tmp = parse_disj(args, pq);
                rv = predicate_AND(rv, tmp);
	}
        return rv;
}

/* predicate -> conj files
   files ->
   files -> TOK_STR files
*/
static void parse_predicate(struct arguments * args)
{
	args->toks_pos = 0;

        struct predicate_qualifiers pq;
        pq.fields = strlist_new();
        if (pq.fields == 0) enomem(0);
        pq.mm = M_SUBSTR;
        pq.ignore_case = false;
        pq.whole_pkg = false;

	args->p = parse_conj(args, pq);

        strlist_free(pq.fields);

        while (peek_token(args) == TOK_STR) {
                if (args->num_fnames >= MAX_FNAMES) {
                        message(L_FATAL, 0, _("too many file names"));
                        fail();
                }
                char * s = get_string(args);
                args->fname[args->num_fnames++] =
                        (struct ifile){ .mode = m_read, .s = s };
        }
	if (peek_token(args) != TOK_EOD) {
                message(L_FATAL, 0,
                        _("file names are not allowed within the filter"));
                fail();
        }
}

static void show_field(struct arguments *args,
                       struct paragraph *para,
                       struct field_attr *fa)
{
        struct field_data fds =
                find_field_wr(para,
                              fa->inx,
                              GET_BACKUP_FIELD(fa->application_data));
        for (struct field_datum *fd = fds.first; fd != NULL; fd = fd->next) {
                struct fsaf_read_rv r =
                        fsaf_read(para->common->fp,
                                  fd->start, fd->end - fd->start);

                if (args->short_descr &&
                    fa == description_attr) {
                        char * nl = memchr(r.b, '\n', r.len);
                        if (nl != 0) r.len = nl - r.b;
                }

                if (r.len == 0) {
                        /* don't display a field with an empty value */
                        break;
                }

                if (args->show_field_name) {
                        struct fsaf_read_rv rn =
                                fsaf_read(para->common->fp,
                                          fd->name_start,
                                          fd->name_end - fd->name_start);
                        fwrite(rn.b, 1, rn.len, stdout);
                        fputs(": ", stdout);
                }

                fwrite(r.b, 1, r.len, stdout);
                puts("");
        }
}

static struct argp argp = { .options = options,
			    .parser = parse_opt,
			    .args_doc = argsdoc,
			    .doc = progdoc };

int main (int argc, char * argv[])
{
	setlocale(LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	fieldtrie_init();

	static struct arguments args;
	args.show_field_name = true;
	msg_set_progname(argv[0]);
	description_attr = fieldtrie_insert(description);
	argp_parse (&argp, argc, argv, ARGP_IN_ORDER, 0, &args);
        if (debug_optparse) {
                fflush(stderr);
                fputs("tokens:", stdout);
                for (int i = 0; i < args.toks_np; i++) {
                        putchar(' ');
                        fputs(tokdescr(args.toks[i]), stdout);
                        if (args.strings[i] != 0) {
                                printf("[%s]", args.strings[i]);
                        }
                }
                puts("");
                fflush(stdout);
        }

	parse_predicate(&args);

	if (debug_optparse) { dump_args(&args); return 0; }

	if (!check_predicate(args.p)) {
		message(L_FATAL, 0, _("malformed filter"));
		fail();
	}

	if (args.short_descr && !args.description_selected) {
		if (args.num_show_fields >= MAX_FIELDS) {
			message(L_FATAL, 0, _("too many output fields"));
			fail();
		}
		message(L_INFORMATIONAL, 0,
			_("Adding \"Description\" to selected output fields because of -d"));
                SET_SHOW_FIELD(description_attr->application_data, 1);
                args.show_fields[args.num_show_fields] = description_attr->inx;
		++args.num_show_fields;
	}

        if (args.invert_show && args.num_show_fields == 0) {
                message(L_FATAL, 0,
                        _("-I requires at least one instance of -s"));
                fail();
        }

	if (!args.show_field_name && args.num_show_fields == 0) {
		message(L_FATAL, 0,
			_("cannot suppress field names when showing whole paragraphs"));
		fail();
	}

	size_t count = 0;
	bool found = false;
	for (size_t i = 0; i < args.num_fnames || (i == 0 && args.num_fnames == 0); ++i) {
		int fd;
		struct ifile fname;
		if (args.num_fnames == 0) {
			char * argv0 = fnbase(argv[0]);
			if (strcmp(argv0, "grep-dctrl") == 0) {
				fname = (struct ifile){ .mode = m_read,
							.s = "-" };
                        } else if (strcmp(argv0, "grep-status") == 0) {
                                fname = (struct ifile){
                                        .mode = m_read,
                                        .s = "/var/lib/dpkg/status" };
                        } else if (strcmp(argv0, "grep-available") == 0) {
                                fname = (struct ifile){
                                        .mode = m_read,
                                        .s = "/var/lib/dpkg/available" };
                        } else if (strcmp(argv0, "grep-aptavail") == 0) {
                                fname = (struct ifile){
                                        .mode = m_exec,
                                        .s = "apt-cache dumpavail" };
                        } else if (strcmp(argv0, "grep-debtags") == 0) {
                                fname = (struct ifile){
                                        .mode = m_exec,
                                        .s = "debtags dumpavail" };
			} else {
                                message(L_FATAL, 0,
                                        _("executable name '%s' is not recognised"),
                                        argv0);
                                fail();
			}
		} else {
			fname = args.fname[i];
		}

		if (fname.mode == m_error) break;

		fd = open_ifile(fname);
		if (fd == -1) break;

		if (!chk_ifile(fname, fd)) break;

		FSAF * fp = fsaf_fdopen(fd, fname.s);
		para_parser_t pp;
		para_parser_init(&pp, fp, true, args.ignore_errors,
                                 args.invert_show);
		para_t para;
		para_init(&pp, &para);
		while (1) {
			para_parse_next(&para);
			if (para_eof(&pp)) break;
			if ((args.invert_match 
			     || !does_para_satisfy(args.p, &para))
			    && (!args.invert_match 
				|| does_para_satisfy(args.p, &para))) {
				continue;
			}
			if (args.quiet) {
				exit(0);
			}
			found = true;
			if (args.count) {
				++count;
				continue;
			}
			if (args.num_show_fields == 0) {
				struct fsaf_read_rv r = get_whole_para(&para);
				fwrite(r.b, 1, r.len, stdout);
				putchar('\n');
				putchar('\n');
				continue;
			}
                        if (args.invert_show) {
                                for (size_t j = 0;
                                     j < fieldtrie_count() &&
                                             j < para.nfields; 
                                     j++) {
                                        struct field_attr *fa = 
                                                fieldtrie_get(j);
                                        if (IS_SHOW_FIELD(fa->application_data)) {
                                                continue;
                                        }
                                        show_field(&args, &para, fa);
                                }
                        } else {
                                for (size_t j = 0;
                                     j < args.num_show_fields; j++) {
                                        size_t inx = args.show_fields[j];
                                        struct field_attr *fa = 
                                                fieldtrie_get(inx);
                                        assert(IS_SHOW_FIELD
                                               (fa->application_data));
                                        show_field(&args, &para, fa);
                                }
                                if ((args.show_field_name &&
                                     args.ensure_dctrl) ||
                                    args.num_show_fields > 1) puts("");
                        }
		}

		fsaf_close(fp);
		close_ifile(fname, fd);
	}
	if (args.count) printf("%zi\n", count);
	return errors_reported() ? 2 : found || args.count ? 0 : 1;
}

