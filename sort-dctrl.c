/*  dctrl-tools - Debian control file inspection tools
    Copyright (C) 2004, 2005 Antti-Juhani Kaijanaho

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

#define OPT_SILENT 256
#define OPT_MMAP 257

const char * argp_program_version = "sort-dctrl (dctrl-tools) " VERSION;
const char * argp_program_bug_address = MAINTAINER;

static struct argp_option options[] = {
	{ "copying",	       'C', 0,		    0, N_("Print out the copyright license.") },
	{ "errorlevel",	       'l', N_("LEVEL"),    0, N_("Set debugging level to LEVEL.") },
	{ "key-spec",          'k', N_("KEYSPEC"),  0, N_("Specify sort keys.") },
	{ "mmap",               OPT_MMAP, 0,        0, N_("Attempt mmapping input files") },
	{ 0 }
};

#define MAX_FNAMES 4096

struct arguments {
	/* Sort key specifications */
	keys_t keys;
	/* Number of file names seen.  */
	size_t num_fnames;
	/* File names seen on the command line.  */
	struct ifile fname[MAX_FNAMES];
};

static error_t parse_opt (int key, char * arg, struct argp_state * state)
{
	struct arguments * args = state->input;
	switch (key) {
	case 'C':
		if (!to_stdout (COPYING)) fail();
		exit(0);
	case 'k':
	{
		char *carg = strdup(arg);
		if (carg == 0) fatal_enomem(0);
		for (char *s = strtok(carg, ","); s != 0; s = strtok(0, ",")) {
			char *flags = strrchr(s, ':');
			if (flags == 0) {
				flags = "";
			} else {
				*(flags++) = '\0';
			}
			struct key key;
			key.field_inx = fieldtrie_insert(carg);
			key.type = FT_STRING;
			key.reverse = false;
			for (char *p = flags; *p != '\0'; p++) {
				switch (*p) {
				case 'r':
					key.reverse = true;
					break;
				case 'v': case 'n':
					key.type = FT_VERSION;
					break;
				default:
					message(L_FATAL, _("invalid key flag"),
						0);
					fail();
				}
			}
			keys_append(&args->keys, key);
		}
		break;
	}
	case 'l': 
	{
		int ll = str2loglevel(optarg);
		if (ll < 0)
		{
			message(L_FATAL, _("no such log level"), optarg);
			fail();
		}
		set_loglevel(ll);
		debug_message("parse_opt: l", 0);
	}
		break;
	case OPT_MMAP:
		debug_message("parse_opt: mmap", 0);
		fsaf_mmap = 1;
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

static char progdoc [] = N_("sort-dctrl -- sort Debian control files");

static struct argp argp = { options, parse_opt, 0, progdoc };

int main(int argc, char * argv[])
{
	setlocale(LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	fieldtrie_init();

	static struct arguments args;
	keys_init(&args.keys);
	msg_set_progname(argv[0]);
	argp_parse (&argp, argc, argv, ARGP_IN_ORDER, 0, &args);

	if (args.keys.nks == 0) {
		size_t inx = fieldtrie_insert("Package");
		keys_append(&args.keys, (struct key){ .field_inx = inx,
						      .type = FT_STRING,
						      .reverse = false });
	}

	struct para_bundle pb;
	bundle_init(&pb);

	for (size_t i = 0; i < args.num_fnames; i++) {
		bundle_slurp(&pb, args.fname[i]);
	}

	sort_bundle(&args.keys, &pb);
	size_t num_paras = bundle_size(&pb);
	para_t ** paras = bundle_vec(&pb);
	for (size_t i = 0; i < num_paras; i++) {
		struct fsaf_read_rv r = get_whole_para(paras[i]);
		fwrite(r.b, 1, r.len, stdout);
		putchar('\n');
	}
	return 0;
}
