/*   grep-dctrl - grep Debian control files
     Copyright (C) 1999, 2003, 2004  Antti-Juhani Kaijanaho

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 2 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; see the file COPYING.  If not, write to
     the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
     Boston, MA 02111-1307, USA.

     The author can be reached via mail at (ISO 8859-1 charset for the city)
        Antti-Juhani Kaijanaho
        Helokantie 1 A 16
        FIN-40640 JYVÄSKYLÄ
        FINLAND
        EUROPE
     and via electronic mail from
        gaia@iki.fi
     If you have a choice, use the email address; it is more likely to
     stay current.

*/

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "msg.h"
#include "fnutil.h"
#include "getaline.h"
#include "rc.h"
#include "strutil.h"
#include "util.h"


const char * find_ifile_by_exename(const char * exename, const char * rcfname)
{
	static const char * default_rcfiles [] = {
		/* Order is significant: earlier files override later ones. */
		"~/.grep-dctrlrc",
		SYSCONF "/grep-dctrl.rc",
		0 };
	int i;
	const char * rv = 0;
	char * fname;
	int lineno;
	FILE * f;

	assert(exename != 0);

	if (rcfname == 0) {
		for (i = 0; rv == 0 && default_rcfiles [i] != 0; i++) {
			rv = find_ifile_by_exename(exename, default_rcfiles [i]);
		}
		return rv;
	}

	assert(rcfname != 0);

	fname = fnqualify(rcfname);
	if (fname == 0) {
		errno_msg(L_IMPORTANT, rcfname);
		return 0;
	}

	message(L_INFORMATIONAL, _("reading config file"), fname);

	f = fopen(fname, "r");
	if (f == 0) {
		message(L_INFORMATIONAL, strerror(errno), fname);
		return 0;
	}

	lineno = 0;
	while (1) {
		static char * line = 0;
		char * line_exe;
		char * line_ifile;

		/* If this is not the first call, line may be non-null
		   and must be freed.  It must be freed on all
		   non-first iterations, too. */
		free(line);

		line = getaline (f);
		if (line == 0) {
			message(L_FATAL, _("read failure or out of memory"),
				fname);
			fail();
		}

		++lineno;
		if (line == 0) {
			rv = 0;
			break;
		}

		chop_comment(line, '#');

		if (left_trimmed(line) [0] == 0) {
			continue;
		}

		line_exe = strtok(line, " \t");
		if (line_exe == 0) {
			line_message(L_IMPORTANT,
				     _("syntax error: need a executable name"),
				     fname, lineno);
			continue;
		}

		line_ifile = strtok(0, " \t");
		if (line_ifile == 0) {
			line_message(L_IMPORTANT,
				     _("syntax error: need an input file name"),
				     fname, lineno);
			continue;
		}

		message(L_INFORMATIONAL, _("considering executable name"), line_exe);
		if (strcmp (exename, line_exe) == 0) {
			message(L_INFORMATIONAL, _("yes, will use executable name"), line_exe);
			rv = line_ifile;
			message(L_INFORMATIONAL, _("default input file"), rv);
			break;
		}
	}

	fclose(f);
	free(fname);
	return rv;
}

