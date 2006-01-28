/*  dctrl-tools - Debian control file inspection tools
    Copyright (C) 1999, 2000, 2001, 2002, 2003 Antti-Juhani Kaijanaho

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
#include <fcntl.h>
#include <publib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fsaf.h"
#include "msg.h"
#include "paragraph.h"
#include "predicate.h"
#include "rc.h"
#include "util.h"

const char * argp_program_version = "grep-dctrl (dctrl-tools) " VERSION;
const char * argp_program_bug_address = MAINTAINER;

const char description [] = "Description";
size_t description_inx;

static char progdoc [] = "grep-dctrl -- grep Debian control files";
     
#define OPT_CONFIG 256
#define OPT_OPTPARSE 257

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

	if (!automatic) exit(EXIT_SUCCESS);

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

static struct argp_option options[] = {
	{ "banner",         'B', 0,                 0, "Show the testing banner." },
	{ "errorlevel",     'l', "LEVEL",           0, "Set debugging level to LEVEL." },
	{ "field",          'F', "FIELD,FIELD,...", 0, "Restrict  pattern  matching  to  the FIELDs given." },
	{ 0,                'P', 0,                 0, "Shorthand for -FPackage" },
	{ "show-field",     's', "FIELD,FIELD,...", 0, "Show only the body of these fields from the matching paragraphs." },
	{ 0,                'd', 0,                 0, "Show only the first line of the Description field from the matching paragraphs." },
	{ "no-field-names", 'n', 0,                 0, "Suppress field names when showing specified fields." },
	{ "eregex",         'e', 0,                 0, "Regard the pattern as an extended POSIX regular expression." },
	{ "regex",          'r', 0,                 0, "The pattern is a standard POSIX regular expression." },
	{ "ignore-case",    'i', 0,                 0, "Ignore case when looking for a match." },
	{ "invert-match",   'v', 0,                 0, "Show only paragraphs that do not match." },
	{ "count",          'c', 0,                 0, "Show only the count of matching paragraphs." },
	{ "config-file",    OPT_CONFIG, "FNAME",    0, "Use FNAME as the config file." },
	{ "exact-match",    'X', 0,                 0, "Do an exact match." },
	{ "copying",        'C', 0,                 0, "Print out the copyright license." },
	{ "and",            'a', 0,                 0, "Conjunct predicates." },
	{ "or",             'o', 0,                 0, "Disjunct predicates." },
	{ "not",            '!', 0,                 0, "Negate the following predicate." },
	{ "debug-optparse", OPT_OPTPARSE, 0,        0, "Debug option parsing." },
	{ 0 }
};


enum state { STATE_ATOM, STATE_NEG, STATE_CONJ, STATE_DISJ, STATE_PAREN, 
	     STATE_START, STATE_FINISHED };

#define MAX_FNAMES 4096

static int debug_optparse = 0;

struct arguments {
	/* Parser state, used when parsing the predicate. */
	enum state state;
	/* Top of the parser stack.  */
	size_t top;
	/* Number of file names seen.  */
	size_t num_fnames;
	/**/
	size_t num_show_fields;
	/* A machine-readable representation of the predicate.  */
	struct predicate p;
	/* Configuration file name */
	char const * rcname;
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
	/* Parser stack.  */
	struct {
		enum state state;
		int insn;
	} stack[MAX_OPS];
	/* File names seen on the command line.  */
	char const * fname[MAX_FNAMES];
	/**/
	struct show_fields {
		char const * name;
		size_t inx;
	} show_fields[MAX_FIELDS];
};

static void finish_atom(struct arguments * args)
{
	struct atom * atom = get_current_atom(&args->p);
	if (atom->pat == 0) {
		message(L_FATAL, "A pattern is mandatory.", 0);
		exit(EXIT_FAILURE);
	}
	predicate_finish_atom(&args->p);
}

/* Pop off one stack state, inserting the associated instruction to
 * the predicate program.  If paren is true, current state must be
 * STATE_PAREN, and if paren is false, it must not be STATE_PAREN. */
static void leave(struct arguments * args, int paren)
{
	debug_message("leaving...", 0);
	assert(paren == (args->state == STATE_PAREN));
	if (args->state == STATE_ATOM) finish_atom(args);
	assert(args->top > 0);
	--args->top;
	addinsn(&args->p, args->stack[args->top].insn);
	args->state = args->stack[args->top].state;
}

#define ENTER(state,insn) do { enter(args, (state), (insn)); } while (0)

static void prim_enter(struct arguments * args, const enum state state, const int insn)
{
	if (args->top >= MAX_OPS) {
		message(L_FATAL, "predicate is too complex", 0);
		exit(EXIT_FAILURE);
	}
	args->stack[args->top].insn = insn;
	args->stack[args->top].state = args->state;
	++args->top;
	args->state = state;
}

/* Push current state along with the given instruction to stack and
 * enter the given state.
 */
static void enter(struct arguments * args, const enum state state, const int insn)
{
	if (args->state == STATE_FINISHED) {
		message(L_FATAL, "syntax error in command line", 0);
		exit(EXIT_FAILURE);
	}
	while (args->state < state || (state != STATE_NEG && args->state == state)) {
		leave(args, 0);
	}
	prim_enter(args, state, insn);
	debug_message("entering...", 0);
}

#define FINISH do { finish(args); } while (0)

/* Flush the state stack. */
static void finish(struct arguments * args)
{
	while (args->top > 0) {
		if (args->state == STATE_PAREN) {
			message(L_FATAL, "missing ')' in command line", 0);
			exit(EXIT_FAILURE);
		}
		leave(args, 0);
	}
	assert(args->state == STATE_START);
	args->state = STATE_FINISHED;
}

#define ENTER_ATOM (enter_atom((args)))

/* If necessary, enter STATE_ATOM and allocate a new atom, pushing
 * along with the old state a PUSH instruction for the new atom to the
 * parser stack.  If we are already in STATE_ATOM, reuse the current
 * atom. */
static struct atom * enter_atom(struct arguments * args)
{
	struct atom * rv;
	if (args->state == STATE_ATOM || args->state == STATE_FINISHED) {
		assert(args->p.num_atoms > 0);
		return &args->p.atoms[args->p.num_atoms-1];
	}
	if (args->p.num_atoms >= MAX_ATOMS) {
		message(L_FATAL, "predicate is too complex", 0);
		exit(EXIT_FAILURE);
	}
	ENTER(STATE_ATOM, I_PUSH(args->p.num_atoms));
	rv = &args->p.atoms[args->p.num_atoms++];
	rv->field_name = 0;
	rv->field_inx = -1;
	rv->mode = M_SUBSTR;
	rv->ignore_case = 0;
	rv->pat = 0;
	rv->patlen = 0;
	return rv;
}

static error_t parse_opt (int key, char * arg, struct argp_state * state)
{
	struct arguments * args = state->input;
	struct atom * atom;
	debug_message("parse_opt", 0);
	switch (key) {
	case 'B':
		banner(false);
	case 'v':
		args->invert_match = true;
		break;
	case 'c':
		args->count = true;
		break;
	case 'n':
		debug_message("parse_opt: n", 0);
		args->show_field_name = false;
		break;
	case 'd':
		args->short_descr = true;
		break;
	case 's': {
		char * carg = strdup(arg);
		if (carg == 0) fatal_enomem(0);
		for (char * s = strtok(carg, ","); s != 0; s = strtok(0, ",")) {
			struct show_fields * sf = &args->show_fields[args->num_show_fields];
			sf->name = strdup(s);
			if (sf->name == 0) fatal_enomem(0);
			sf->inx = fieldtrie_insert(&args->p.trie, s);
			if (sf->inx == description_inx) {
				args->description_selected = true;
			}
			++args->num_show_fields;
		}
	}
		break;
	case 'l': {
		int ll = str2loglevel(optarg);
		if (ll < 0)
		{
			message(L_FATAL, _("no such log level"), optarg);
			exit(EXIT_FAILURE);
		}
		set_loglevel(ll);
		debug_message("parse_opt: l", 0);
	}
		break;
	case '!':
		debug_message("parse_opt: !", 0);
		ENTER(STATE_NEG, I_NEG);
		break;
	case 'a':
		debug_message("parse_opt: a", 0);
		ENTER(STATE_CONJ, I_AND);
		break;
	case 'o':
		debug_message("parse_opt: o", 0);
		ENTER(STATE_DISJ, I_OR);
		break;
	case 'P':
		debug_message("parse_opt: P", 0);
		arg = "Package";
		/* pass through */
	case 'F':
		debug_message("parse_opt: F", 0);
		atom = ENTER_ATOM;
		assert(atom->field_name == 0); /* FIXME */
		atom->field_name = strdup(arg);
		if (atom->field_name == 0) fatal_enomem(0);
		break;
	case 'X':
		debug_message("parse_opt: X", 0);
		atom = ENTER_ATOM;
		assert(atom->mode == M_SUBSTR); /* FIXME */
		atom->mode = M_EXACT;
		break;
	case 'r':
		debug_message("parse_opt: r", 0);
		atom = ENTER_ATOM;
		assert(atom->mode == M_SUBSTR); /* FIXME */
		atom->mode = M_REGEX;
		break;
	case 'e':
		debug_message("parse_opt: e", 0);
		atom = ENTER_ATOM;
		assert(atom->mode == M_SUBSTR); /* FIXME */
		atom->mode = M_EREGEX;
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
	case ARGP_KEY_ARG: 
		debug_message("parse_opt: argument", 0);
	redo:
		debug_message("!!!", 0);
		if (strcmp(arg, "!") == 0) {
			debug_message("parse_opt: !", 0);
			ENTER(STATE_NEG, I_NEG);
			break;
		}
		if (strcmp(arg, "(") == 0) {
			debug_message("parse_opt: (", 0);
			prim_enter(args, STATE_PAREN, I_NOP);
			break;
		}
		if (strcmp(arg, ")") == 0) {
			debug_message("parse_opt: )", 0);			
			while (args->state != STATE_PAREN) {
				if (args->top == 0) {
					message(L_FATAL, "unexpected ')' in command line", 0);
					exit(EXIT_FAILURE);
				}
				leave(args, 0);
			}
			leave(args, 1);
			break;
		}
		if (args->state == STATE_FINISHED) {
			char const * s;
			if (args->num_fnames >= MAX_FNAMES) {
				message(L_FATAL, "too many file names", 0);
				exit(EXIT_FAILURE);
			}
			s = strdup(arg);
			if (s == 0) fatal_enomem(0);
			args->fname[args->num_fnames++] = s;
			break;
		}
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
		if (args->state != STATE_FINISHED) FINISH;
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
	assert(args->state == STATE_FINISHED);
	assert(args->top == 0);
	printf("num_atoms = %zi\n", args->p.num_atoms);
	for (i = 0; i < args->p.num_atoms; i++) {
		printf("atoms[%zi].field_name = %s\n", i, args->p.atoms[i].field_name);
		printf("atoms[%zi].mode = %i\n", i, args->p.atoms[i].mode);
		printf("atoms[%zi].ignore_case = %i\n", i, args->p.atoms[i].ignore_case);
		printf("atoms[%zi].pat = %s\n", i, args->p.atoms[i].pat);
	}
	printf("proglen = %zi\n", args->p.proglen);
	for (i = 0; i < args->p.proglen; i++) {	
		int op = args->p.program[i];
		printf("program[%zi] = ", i);
		switch (op) {
		case I_NOP: puts("NOP"); break;
		case I_NEG: puts("NEG"); break;
		case I_AND: puts("AND"); break;
		case I_OR:  puts("OR"); break;
		default:
			printf("PUSH(%i)\n", op - I_PUSH(0));
		}
	}
	printf("num_fnames = %zi\n", args->num_fnames);
	for (i = 0; i < args->num_fnames; i++) {
		printf("fname[%zi] = %s\n", i, args->fname[i]);
	}
}

static struct argp argp = { options, parse_opt, 0, progdoc };

int main (int argc, char * argv[])
{
	static struct arguments args;
	args.state = STATE_START;
	args.show_field_name = true;
	msg_set_progname(argv[0]);
	init_predicate(&args.p);
	description_inx = fieldtrie_insert(&args.p.trie, description);
	argp_parse (&argp, argc, argv, ARGP_IN_ORDER, 0, &args);
	banner(true);

	if (debug_optparse) { dump_args(&args); return 0; }

	if (args.p.num_atoms == 0) {
		message(L_FATAL, "a predicate is required", 0);
		exit(EXIT_FAILURE);
	}
	
	if (args.short_descr && !args.description_selected) {
		if (args.num_show_fields >= MAX_FIELDS) {
			message(L_FATAL, _("too many output fields"), 0);
			exit(EXIT_FAILURE);
		}
		message(L_INFORMATIONAL, 
			_("Adding Description to selected output fields because of -d"),
			0); 
		args.show_fields[args.num_show_fields].name = description;
		args.show_fields[args.num_show_fields].inx = description_inx;
		++args.num_show_fields;
	}	

	if (!args.show_field_name && args.num_show_fields == 0) {
		message(L_FATAL,
			_("cannot suppress field names when showing whole paragraphs"),
			0);      
		exit(EXIT_FAILURE);
	}

	size_t count = 0;
	for (size_t i = 0; i < args.num_fnames || (i == 0 && args.num_fnames == 0); ++i) {
		int fd;
		const char * fname;
		if (args.num_fnames == 0) {
			// Hardcode grep-dctrl <-> "-" mapping so that
			// Debian packages can genuinely depend on it.
			char * argv0 = fnbase(argv[0]);
			if (strcmp(argv0, "grep-dctrl") == 0) {
				fname = "-";
			} else {
				fname = find_ifile_by_exename(argv0, args.rcname);
			}
		} else {
			fname = args.fname[i];
		}
		
		if (strcmp(fname, "-") == 0) {
			fd = STDIN_FILENO;
			fname = "stdin";
		} else {
			fd = open(fname, O_RDONLY);
			if (fd == -1) {
				fprintf(stderr, "%s: %s: %s\n", argv[0], fname, strerror(errno));
				break;
			}
		}

		FSAF * fp = fsaf_fdopen(fd);
		para_t para;
		for (para_init(&para, fp, &args.p.trie); 
		     !para_eof(&para); 
		     para_parse_next(&para)) {
			if ((args.invert_match || !does_para_satisfy(&args.p, &para))
			    && (!args.invert_match || does_para_satisfy(&args.p, &para))) {
				continue;
			}
			if (args.count) {
				++count;
				continue;
			}
			if (args.num_show_fields == 0) {
				struct fsaf_read_rv r = fsaf_read(fp, para.start, para.end - para.start);
				fwrite(r.b, 1, r.len, stdout);
				putchar('\n');
				continue;
			}
			for (size_t j = 0; j < args.num_show_fields; j++) {
				if (args.show_field_name) {
					printf("%s: ", args.show_fields[j].name);
				}
				struct field_data * fd = &para.fields[args.show_fields[j].inx];
				struct fsaf_read_rv r = fsaf_read(fp, fd->start, fd->end - fd->start);
				if (args.short_descr && 
				    args.show_fields[j].inx == description_inx) {
					char * nl = memchr(r.b, '\n', r.len);
					if (nl != 0) r.len = nl - r.b;
				}
				fwrite(r.b, 1, r.len, stdout);
				puts("");
				continue;
			}
			if (args.num_show_fields > 1) puts("");
				
		}
		
		if (fd != STDIN_FILENO) close(fd);
	}      
	if (count) printf("%d\n", count);
	return 0;
}

