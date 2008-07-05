/*   grep-dctrl - grep Debian control files
     Copyright © 1999, 2003, 2004, 2008  Antti-Juhani Kaijanaho

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

*/

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "msg.h"
#include "fnutil.h"
#include "getaline.h"
#include "rc.h"
#include "strutil.h"
#include "util.h"

char const * const ifile_modes[] = { [m_error] = "m_error",
				     [m_read]  = "m_read",
				     [m_exec]  = "m_exec" };

static struct ifile parse(char * s)
{
	assert(s != 0);
	s = (char*)left_trimmed(s);
	trim_right(s);
	if (*s == 0) return (struct ifile) { .mode = m_read, .s = "-" };
	static const char at_exec[] = "@exec ";
	if (s[0] != '@') return (struct ifile){ .mode = m_read, .s = s };
	assert(s[0] == '@');
	if (s[1] == '@') return (struct ifile){ .mode = m_read, .s = s+1 };
	if (strncmp(s, at_exec, sizeof at_exec - 1) == 0) {
		return (struct ifile){ .mode = m_exec,
				       .s = s + sizeof at_exec - 1 };
	}
	debug_message(s, 0);
	message(L_IMPORTANT, _("Malformed default input file name"), 0);
	return (struct ifile){ .mode = m_error, .s = 0 };
}

static bool perms_ok(char const * fname, int fd)
{
	struct stat stat;
	int r = fstat(fd, &stat);
	if (r == -1) {
		errno_msg(L_IMPORTANT, fname);
		return false;
	}
	if (stat.st_uid != 0 && stat.st_uid != getuid()) {
		message(L_IMPORTANT, _("not owned by you or root, ignoring"),
			fname);
		return false;
	}
	if ((stat.st_mode & (S_IWGRP | S_IWOTH)) != 0) {
		message(L_IMPORTANT, _("write permissions for "
				       "group or others, ignoring"),
			fname);
		return false;
	}
	return true;
}

struct ifile find_ifile_by_exename(const char * exename, const char * rcfname)
{
	static const char * default_rcfiles [] = {
		/* Order is significant: earlier files override later ones. */
		"~/.grep-dctrlrc",
		SYSCONF "/grep-dctrl.rc",
		0 };
	int i;
	char * fname;
	int lineno;
	FILE * f;

	assert(exename != 0);

	if (rcfname == 0) {
		struct ifile rv = { .mode = m_error, .s = 0 };
		for (i = 0;
		     rv.mode == m_error && default_rcfiles [i] != 0;
		     i++) {
			rv = find_ifile_by_exename(exename, default_rcfiles [i]);
		}
		return rv;
	}

	assert(rcfname != 0);

	fname = fnqualify(rcfname);
	if (fname == 0) {
		errno_msg(L_IMPORTANT, rcfname);
		return (struct ifile){ .mode = m_error, .s = 0 };
	}

	message(L_INFORMATIONAL, _("reading config file"), fname);

	f = fopen(fname, "r");
	if (f == 0) {
		message(L_INFORMATIONAL, strerror(errno), fname);
		return (struct ifile){ .mode = m_error, .s = 0 };
	}

	// check permissions
	if (!perms_ok(fname, fileno(f))) {
		return (struct ifile){ .mode = m_error, .s = 0 };
	}

	lineno = 0;
	char * rv = 0;
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

		if (*line == 0) {
			rv = 0;
			break;
		}

		chop_comment(line, '#');
		chomp(line);

		if (left_trimmed(line) [0] == 0) {
			continue;
		}

		line_exe = strtok(line, " \t");
		if (line_exe == 0) {
			line_message(L_IMPORTANT, fname, lineno,
				     _("syntax error: need a executable name")
                                );
			continue;
		}

		line_ifile = strtok(0, "\n\t");
		if (line_ifile == 0) {
			line_message(L_IMPORTANT, fname, lineno,
				     _("syntax error: need an input file name")
                                );
			continue;
		}

		message(L_INFORMATIONAL, line_exe, 
                        _("considering executable name"));
		if (strcmp (exename, line_exe) == 0) {
			message(L_INFORMATIONAL, line_exe,
                                _("yes, will use executable name"));
			rv = line_ifile;
			message(L_INFORMATIONAL, rv, _("default input file"));
			break;
		}
	}

	fclose(f);
	free(fname);

	if (rv != 0) {
		return parse(rv);
	} else {
		message(L_IMPORTANT, 0,
                        _("executable name not found; "
                          "reading from standard input"));
		return (struct ifile) { .mode = m_read, .s = "-" };
	}
}

