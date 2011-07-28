/*  dctrl-tools - Debian control file inspection tools
    Copyright © 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
                2010, 2011
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
#include "fnutil.h"
#include "fsaf.h"
#include "i18n.h"
#include "misc.h"
#include "msg.h"
#include "paragraph.h"
#include "predicate.h"
#include "rc.h"
#include "util.h"

const char * argp_program_version = "grep-dctrl (dctrl-tools) " VERSION;
const char * argp_program_bug_address = MAINTAINER;

const char description [] = "Description";
struct field_attr *description_attr;

static char progdoc [] = N_("grep-dctrl -- grep Debian control files");

static char argsdoc [] = "PREDICATE [FILENAME...]";

enum {
        OPT_CONFIG=256,
        OPT_OPTPARSE,
        OPT_SILENT,
        OPT_EQ,
        OPT_LT,
        OPT_LE,
        OPT_GT,
        OPT_GE,
        OPT_MMAP,
        OPT_IGN_ERRS,
        OPT_PATTERN
};

#undef BANNER

#ifdef BANNER
void banner(bool automatic)
{
	char * fname = fnqualify_xalloc("~/.grep-dctrl-banner-shown");
	struct stat st;
	if (automatic) {
		int r = stat(fname, &st);
		if (r == 0) goto end;
	}
	FILE * fp = fopen("/dev/tty", "w");
	if (fp == 0) {
		perror("/dev/tty");
		goto end;
	}
	fprintf(fp,
		"==========================================================================\n"
		"                                  NOTE                                    \n"
		" grep-dctrl has been rewritten from scratch.  Although this does add new  \n"
		" features, regressions are certainly possible.  Please watch for them and \n"
		" report them to the BTS.                                                  \n"
		"==========================================================================\n"
		"(The above annoying banner will not be shown to you again, unless you\n"
		"request it with the -B switch.  It will also be removed entirely soon.)\n");

	int r = creat(fname, 0644);
	if (r == -1) perror(fname);

	if (!automatic) exit(0);

	for (int i = 15; i > 0; i--) {
		fprintf(fp, "%2d seconds until program is resumed...\r", i);
		fflush(fp);
		sleep(1);
	}
	fprintf(fp, "                                       \r");
	fflush(fp);
end:
	free(fname);
}
#endif

static struct argp_option options[] = {
#ifdef BANNER
	{ "banner",	    'B', 0,		    0, N_("Show the testing banner.") },
#endif
	{ "errorlevel",	    'l', N_("LEVEL"),	    0, N_("Set debugging level to LEVEL.") },
	{ "field",	    'F', N_("FIELD,FIELD,..."), 0, N_("Restrict pattern matching to the FIELDs given.") },
	{ 0,		    'P', 0,		    0, N_("This is a shorthand for -FPackage.") },
	{ 0,		    'S', 0,		    0, N_("This is a shorthand for -FSource:Package.") },
	{ "show-field",	    's', N_("FIELD,FIELD,..."), 0, N_("Show only the body of these fields from the matching paragraphs.") },
	{ 0,		    'd', 0,		    0, N_("Show only the first line of the \"Description\" field from the matching paragraphs.") },
	{ "no-field-names", 'n', 0,		    0, N_("Suppress field names when showing specified fields.") },
	{ "eregex",	    'e', 0,		    0, N_("Regard the pattern as an extended POSIX regular expression.") },
	{ "regex",	    'r', 0,		    0, N_("The pattern is a standard POSIX regular expression.") },
	{ "ignore-case",    'i', 0,		    0, N_("Ignore case when looking for a match.") },
	{ "invert-match",   'v', 0,		    0, N_("Show only paragraphs that do not match.") },
        { "invert-show",    'I', 0,                 0, N_("Show those fields that have NOT been selected with -s") },
	{ "count",	    'c', 0,		    0, N_("Show only the count of matching paragraphs.") },
	{ "config-file",    OPT_CONFIG, N_("FNAME"),0, N_("Use FNAME as the config file.") },
	{ "exact-match",    'X', 0,		    0, N_("Do an exact match.") },
	{ "copying",	    'C', 0,		    0, N_("Print out the copyright license.") },
	{ "and",	    'a', 0,		    0, N_("Conjunct predicates.") },
	{ "or",		    'o', 0,		    0, N_("Disjunct predicates.") },
	{ "not",	    '!', 0,		    0, N_("Negate the following predicate.") },
	{ "eq",		    OPT_EQ, 0,		    0, N_("Test for version number equality.") },
	{ "lt",		    OPT_LT, 0,		    0, N_("Version number comparison: <.") },
	{ "le",		    OPT_LE, 0,		    0, N_("Version number comparison: <=.") },
	{ "gt",		    OPT_GT, 0,		    0, N_("Version number comparison: >.") },
	{ "ge",		    OPT_GE, 0,		    0, N_("Version number comparison: >=.") },
	{ "debug-optparse", OPT_OPTPARSE, 0,	    0, N_("Debug option parsing.") },
	{ "quiet",	    'q', 0,		    0, N_("Do no output to stdout.") },
	{ "silent",	    OPT_SILENT, 0,	    0, N_("Do no output to stdout.") },
	{ "mmap",           OPT_MMAP, 0,            0, N_("Attempt mmapping input files") },
	{ "ignore-parse-errors", OPT_IGN_ERRS, 0,   0, N_("Ignore parse errors") },
        { "pattern",        OPT_PATTERN, N_("PATTERN"), 0, N_("Specify the pattern to search for") },
	{ "whole-pkg",	    'w', 0,                 0, N_("Match only whole package names (this implies -e)") },
	{ 0 }
};


// Tokens
#define TOK_EOD 0
#define TOK_NOT 1
#define TOK_AND 2
#define TOK_OR  3
#define TOK_LP  4 /* left paren */
#define TOK_RP  5 /* right paren */
#define TOK_ATOM_BASE 6 /* All tokens >= TOK_ATOM_BASE are atoms; the
			   difference is the atom index.  */

#define MAX_FNAMES 4096
#define MAX_TOKS   16384

static int debug_optparse = 0;

struct arguments {
	/* Parser state flag: last token seen was ')' */
	bool just_seen_cparen;
	/* Top of the parser stack.  */
	size_t top;
	/* Number of file names seen.  */
	size_t num_fnames;
	/**/
	size_t num_show_fields;
	/**/
	size_t num_search_fields;
	/* A machine-readable representation of the predicate.  */
	struct predicate p;
	/* Configuration file name */
	char const * rcname;
	/* Ignore parse errors? */
	bool ignore_errors;
	/* Quiet operation? */
	bool quiet;
	/* Do show field names? */
	bool show_field_name;
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
	/* Finished with the predicate scanning? */
	bool finished;
	/* Are we inside an atom? */
	bool in_atom;
	/* Pattern error? */
	bool pattern_error;
	/* Token stream for the predicate parser. */
	int toks[MAX_TOKS];
	/* For each atom, give code with which it can be accessed.  */
	struct atom_code {
		size_t n;
		int * routine;
	} *atom_code[MAX_ATOMS];
	/* File names seen on the command line.  */
	struct ifile fname[MAX_FNAMES];
        /**/
        size_t show_fields[MAX_FIELDS];
	/* Search field names seen during current atom.  */
	char * search_fields[MAX_FIELDS];
};

#define IS_SHOW_FIELD(field_app_data) ((field_app_data) & 1)
#define SET_SHOW_FIELD(field_app_data,val) \
  ((field_app_data) = ((field_app_data & ~1) | val))
#define GET_BACKUP_FIELD(field_app_data) (((field_app_data & ~0)) == (unsigned)-1 ? (size_t)-1 : (size_t)(field_app_data) >> 1)
#define SET_BACKUP_FIELD(field_app_data,val) \
  ((field_app_data) = (((field_app_data)&1) | (val<<1)))

struct atom * clone_atom(struct arguments * args)
{
	if (args->p.num_atoms >= MAX_ATOMS) {
		message(L_FATAL, 0, _("predicate is too complex"));
		fail();
	}
	int oa = args->p.num_atoms-1;
	struct atom * atom = get_current_atom(&args->p);
	int na = args->p.num_atoms;
	struct atom * rv = &args->p.atoms[args->p.num_atoms++];
	rv->field_name = atom->field_name;
	rv->field_inx = atom->field_inx;
	rv->mode = atom->mode;
	rv->ignore_case = atom->ignore_case;
	rv->whole_pkg = atom->whole_pkg;
	rv->pat = atom->pat;
	rv->patlen = atom->patlen;
	struct atom_code * ac = args->atom_code[oa];
	args->atom_code[na] = ac;
	assert(ac->n > 0);
	ac->n += 2;
	ac->routine = realloc(ac->routine, ac->n * sizeof *ac->routine);
	if (ac->routine == 0) fatal_enomem(0);
	ac->routine[ac->n-2] = I_PUSH(na);
	ac->routine[ac->n-1] = I_OR;
	return rv;
}

static void finish_atom(struct arguments * args)
{
	assert(args->in_atom);
	args->in_atom = false;
	struct atom * atom = get_current_atom(&args->p);
	if (atom->pat == 0) {
		args->pattern_error = true;
		return;
	}
	for (size_t i = 0; i < args->num_search_fields; i++) {
		if (i > 0) atom = clone_atom(args);
		atom->field_name = args->search_fields[i];
		predicate_finish_atom(&args->p);
	}
	// If there are no fields, we have not yet run this...
	// ... but it must be done (especially with -r/-e atoms)
	if (args->num_search_fields == 0) predicate_finish_atom(&args->p);
	args->num_search_fields = 0;
}

#if 0
/* Pop off one stack state, inserting the associated instructions to
 * the predicate program.  If paren is true, current state must be
 * STATE_PAREN, and if paren is false, it must not be STATE_PAREN. */
static void leave(struct arguments * args, int paren)
{
	debug_message("leaving...", 0);
	assert(paren == (args->state == STATE_PAREN));
	if (args->state == STATE_ATOM) finish_atom(args);
	assert(args->top > 0);
	--args->top;
	for (struct insn_node * it = args->stack[args->top].insns_first;
	     it != 0;) {
		addinsn(&args->p, it->insn);
		struct insn_node * next = it->next;
		free(it);
		it = next;
	}
	args->stack[args->top].insns_first = 0;
	args->stack[args->top].insns_last = 0;
	args->state = args->stack[args->top].state;
}
#endif

#define APPTOK(tok) do { apptok(args, (tok)); } while (0)

static void apptok(struct arguments * args, const int tok)
{
	debug_message("apptok", 0);
        if (args->finished) {
                message(L_FATAL, 0,
                        _("file names are not allowed within the predicate"));
                fail();
        }
	if (args->in_atom && tok < TOK_ATOM_BASE) {
		finish_atom(args);
	}
	if (args->toks_np >= MAX_TOKS) {
		message(L_FATAL, 0, _("predicate is too long"));
		fail();
	}
	args->toks[args->toks_np++] = tok;
}

#define FINISH do { finish(args); } while (0)

/* Flush the state stack. */
static void finish(struct arguments * args)
{
	assert(!args->finished);
	if (args->in_atom) finish_atom(args);
	args->finished = true;
}

#define ENTER_ATOM (enter_atom((args)))

/* FIXME: UPDATE COMMENT
If necessary, enter STATE_ATOM and allocate a new atom, pushing
 * along with the old state a PUSH instruction for the new atom to the
 * parser stack.  If we are already in STATE_ATOM, reuse the current
 * atom. */
static struct atom * enter_atom(struct arguments * args)
{
	struct atom * rv;
	if (args->in_atom) {
		assert(args->p.num_atoms > 0);
		return &args->p.atoms[args->p.num_atoms-1];
	}
	args->in_atom = true;
	if (args->p.num_atoms >= MAX_ATOMS) {
		message(L_FATAL, 0, _("predicate is too complex"));
		fail();
	}
	APPTOK(args->p.num_atoms + TOK_ATOM_BASE);
	args->atom_code[args->p.num_atoms] =
		malloc(sizeof *args->atom_code[args->p.num_atoms]);
	if (args->atom_code[args->p.num_atoms] == 0) fatal_enomem(0);
	args->atom_code[args->p.num_atoms]->n = 1;
	args->atom_code[args->p.num_atoms]->routine = malloc(1 * sizeof(int));
	if (args->atom_code[args->p.num_atoms]->routine == 0) {
		fatal_enomem(0);
	}
	args->atom_code[args->p.num_atoms]->routine[0] 
		= I_PUSH(args->p.num_atoms);
	rv = &args->p.atoms[args->p.num_atoms++];
	rv->field_name = 0;
	rv->field_inx = -1;
	rv->mode = M_SUBSTR;
	rv->ignore_case = 0;
	rv->whole_pkg = 0;
	rv->pat = 0;
	rv->patlen = 0;
	return rv;
}

#define set_mode(nmode) do { \
	atom = ENTER_ATOM; \
	if (atom->mode != M_SUBSTR) { \
		message(L_FATAL, 0, _("inconsistent atom modifiers")); \
		fail(); \
	} \
	atom->mode = (nmode); \
} while (0)

static error_t parse_opt (int key, char * arg, struct argp_state * state)
{
	struct arguments * args = state->input;
	bool just_seen_cparen = args->just_seen_cparen;
	args->just_seen_cparen = false;
	struct atom * atom;
	debug_message("parse_opt", 0);
#ifdef INCLUDE_DEBUG_MSGS
		if (do_msg(L_DEBUG)) {
			fprintf(stderr, "%s: in_atom = %s\n",
				get_progname(),
				args->in_atom ? "true" : "false");
		}
#endif
	switch (key) {
	case 'C':
		if (!to_stdout (COPYING)) fail();
		exit(0);
#ifdef BANNER
	case 'B':
		banner(false);
#endif
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
		APPTOK(I_OR);
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
	case 'F': {
		debug_message("parse_opt: Fv", 0);
		atom = ENTER_ATOM;
		char * carg = strdup(arg);
		if (carg == 0) fatal_enomem(0);
		for (char * s = strtok(carg, ","); s != 0; s = strtok(0, ",")){
			char * tmp = strdup(s);
			if (tmp == 0) fatal_enomem(0);
			args->search_fields[args->num_search_fields++] = tmp;
		}
		free(carg);
	}
		break;
	case 'X':
		debug_message("parse_opt: X", 0);
		set_mode(M_EXACT);
		break;
	case 'r':
		debug_message("parse_opt: r", 0);
		set_mode(M_REGEX);
		break;
	case 'e':
		debug_message("parse_opt: e", 0);
		set_mode(M_EREGEX);
		break;
	case OPT_EQ:
		debug_message("parse_opt: eq", 0);
		set_mode(M_VER_EQ);
		break;
	case OPT_LT:
		debug_message("parse_opt: lt", 0);
		set_mode(M_VER_LT);
		break;
	case OPT_LE:
		debug_message("parse_opt: le", 0);
		set_mode(M_VER_LE);
		break;
	case OPT_GT:
		debug_message("parse_opt: gt", 0);
		set_mode(M_VER_GT);
		break;
	case OPT_GE:
		debug_message("parse_opt: ge", 0);
		set_mode(M_VER_GE);
		break;
	case OPT_MMAP:
		debug_message("parse_opt: mmap", 0);
		fsaf_mmap = 1;
		break;
	case 'i':
		debug_message("parse_opt: i", 0);
		atom = ENTER_ATOM;
		atom->ignore_case = 1;
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
		atom = ENTER_ATOM;
		if (atom->pat != 0) {
                        message(L_FATAL, 0, _("Multiple patterns for the same "
                                              "atom are not allowed"));
                        fail();
                }
		atom->patlen = strlen(arg);
		atom->pat = malloc(atom->patlen+1);
		if (atom->pat == 0) fatal_enomem(0);
		strcpy((char*)atom->pat, arg);
		break;
	case 'w':
		debug_message("parse_opt: whole-pkg", 0);
		atom = ENTER_ATOM;
		atom->whole_pkg = 1;
		set_mode(M_EREGEX);
		break;
	case ARGP_KEY_ARG:
		debug_message("parse_opt: argument", 0);
	redo:
		debug_message("!!!", 0);
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
			args->just_seen_cparen = true;
			APPTOK(TOK_RP);
			break;
		}
		if (args->finished) {
			char const * s;
			if (args->num_fnames >= MAX_FNAMES) {
				message(L_FATAL, 0, _("too many file names"));
				fail();
			}
			s = strdup(arg);
			if (s == 0) fatal_enomem(0);
			args->fname[args->num_fnames++] =
				(struct ifile){ .mode = m_read, .s = s };
			break;
		}
		if (just_seen_cparen) { FINISH; goto redo; }
		if (strcmp(arg, "--") == 0) { FINISH; break; }
		atom = ENTER_ATOM;
		if (atom->pat != 0) { FINISH; goto redo; }
		atom->patlen = strlen(arg);
		atom->pat = malloc(atom->patlen+1);
		if (atom->pat == 0) fatal_enomem(0);
		strcpy((char*)atom->pat, arg);
		break;
	case ARGP_KEY_END:
		debug_message("parse_opt: end", 0);
		if (!args->finished) FINISH;
		break;
	case ARGP_KEY_ARGS:  case ARGP_KEY_INIT: case  ARGP_KEY_SUCCESS:
	case ARGP_KEY_ERROR: case ARGP_KEY_FINI: case ARGP_KEY_NO_ARGS:
		debug_message("parse_opt: ignored", 0);
		break;
	case OPT_CONFIG:
		debug_message("parse_opt: --config-file", 0);
		args->rcname = strdup(arg);
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static void dump_args(struct arguments * args)
{
	size_t i;
	assert(args->finished);
	assert(args->top == 0);
	printf("num_atoms = %zi\n", args->p.num_atoms);
	for (i = 0; i < args->p.num_atoms; i++) {
		printf("atoms[%zi].field_name = %s\n", i, args->p.atoms[i].field_name);
		printf("atoms[%zi].mode = %i\n", i, args->p.atoms[i].mode);
		printf("atoms[%zi].ignore_case = %i\n", i, args->p.atoms[i].ignore_case);
		printf("atoms[%zi].whole_pkg = %i\n", i, args->p.atoms[i].whole_pkg);
		printf("atoms[%zi].pat = %s\n", i, args->p.atoms[i].pat);
	}
	printf("proglen = %zi\n", args->p.proglen);
	for (i = 0; i < args->p.proglen; i++) {
		int op = args->p.program[i];
		printf("program[%zi] = ", i);
		switch (op) {
		case I_NOP:  puts("NOP"); break;
		case I_NEG:  puts("NEG"); break;
		case I_AND:  puts("AND"); break;
		case I_OR:   puts("OR"); break;
		default:
			printf("PUSH(%i)\n", op - I_PUSH(0));
		}
	}
	printf("num_fnames = %zi\n", args->num_fnames);
	for (i = 0; i < args->num_fnames; i++) {
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

static void unexpected(int tok)
{
	switch (tok) {
	case TOK_EOD:
		message(L_FATAL, 0, _("unexpected end of predicate"));
		fail();
	case TOK_NOT: 
		message(L_FATAL, 0, _("unexpected '!' in command line"));
		fail();
	case TOK_AND:
		message(L_FATAL, 0, _("unexpected '-a' in command line"));
		fail();
	case TOK_OR : 
		message(L_FATAL, 0, _("unexpected '-o' in command line"));
		fail();
	case TOK_LP :
		message(L_FATAL, 0, _("unexpected '(' in command line"));
		fail();
	case TOK_RP :
		message(L_FATAL, 0, _("unexpected ')' in command line"));
		fail();
	default:
		assert(tok >=TOK_ATOM_BASE);
		message(L_FATAL, 0, _("unexpected atom in command line"));
		fail();
	}
}

static void parse_conj(struct arguments * args);

static void parse_prim(struct arguments * args)
{
	if (peek_token(args) == TOK_LP) {
		get_token(args);
		parse_conj(args);
		if (get_token(args) != TOK_RP) {
			message(L_FATAL, 0, _("missing ')' in command line"));
			fail();
		}
		return;
	}
	if (peek_token(args) < TOK_ATOM_BASE) unexpected(peek_token(args));
	int atom = get_token(args) - TOK_ATOM_BASE;
	assert(atom >= 0);
	assert(atom < MAX_ATOMS);
	struct atom_code *ac = args->atom_code[atom];
	for (size_t i = 0; i < ac->n; i++) {
		addinsn(&args->p, ac->routine[i]);
	}
/*
	addinsn(&args->p, I_PUSH(atom));
*/
}

static void parse_neg(struct arguments * args)
{
	bool neg = false;
	if (peek_token(args) == TOK_NOT) {
		neg = true;
		get_token(args);
	}
	parse_prim(args);
	if (neg) addinsn(&args->p, I_NEG);
}

static void parse_disj(struct arguments * args)
{
	parse_neg(args);
	while (peek_token(args) == TOK_OR) {
		get_token(args);
		parse_neg(args);
		addinsn(&args->p, I_OR);
	}
}

static void parse_conj(struct arguments * args)
{
	parse_disj(args);
	while (peek_token(args) == TOK_AND) {
		get_token(args);
		parse_disj(args);
		addinsn(&args->p, I_AND);
	}
}

static void parse_predicate(struct arguments * args)
{
	args->toks_pos = 0;
	parse_conj(args);
	int tok = peek_token(args);
	if (tok != TOK_EOD) unexpected(tok);
}

static void show_field(struct arguments *args,
                       struct paragraph *para,
                       struct field_attr *fa)
{
        struct field_data *fd =
                find_field_wr(para,
                              fa->inx,
                              GET_BACKUP_FIELD(fa->application_data));
        if (fd == NULL) return;
        struct fsaf_read_rv r =
                fsaf_read(para->common->fp, fd->start, fd->end - fd->start);

        if (args->short_descr &&
            fa == description_attr) {
                char * nl = memchr(r.b, '\n', r.len);
                if (nl != 0) r.len = nl - r.b;
        }

        if (r.len == 0) {
                /* don't display a field with an empty value */
                return;
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
	init_predicate(&args.p);
	description_attr = fieldtrie_insert(description);
	argp_parse (&argp, argc, argv, ARGP_IN_ORDER, 0, &args);
#ifdef BANNER
	banner(true);
#endif
	parse_predicate(&args);
	if (args.pattern_error) {
		message(L_FATAL, 0, _("A pattern is mandatory"));
		fail();
	}

	if (debug_optparse) { dump_args(&args); return 0; }

	if (args.p.num_atoms == 0) {
		message(L_FATAL, 0, _("a predicate is required"));
		fail();
	}

	if (!check_predicate(&args.p)) {
		message(L_FATAL, 0, _("malformed predicate"));
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
			// Hardcode grep-dctrl <-> "-" mapping so that
			// Debian packages can genuinely depend on it.
			char * argv0 = fnbase(argv[0]);
			if (strcmp(argv0, "grep-dctrl") == 0) {
				fname = (struct ifile){ .mode = m_read,
							.s = "-" };
			} else {
				fname = find_ifile_by_exename(argv0, args.rcname);
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
			     || !does_para_satisfy(&args.p, &para))
			    && (!args.invert_match 
				|| does_para_satisfy(&args.p, &para))) {
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
                        }
                        /* let's see how many users howl in pain after
                           deactivating this conditional (see BTS #525525)

                           if (args.num_show_fields > 1)*/ puts("");
		}

		fsaf_close(fp);
		close_ifile(fname, fd);
	}
	if (count) printf("%zi\n", count);
	return errors_reported() ? 2 : found ? 0 : 1;
}

