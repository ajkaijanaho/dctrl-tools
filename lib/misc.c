/*  dctrl-tools - Debian control file inspection tools
    Copyright © 2004, 2008 Antti-Juhani Kaijanaho

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

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include "misc.h"
#include "msg.h"

int to_stdout (const char * fname)
{
	FILE * f;
	FILE * t = stdout;
	bool pipe = false;
	int c;
	int rv = 1;

	if (isatty(STDOUT_FILENO)) {
		char * cmd = getenv("PAGER");
		if (cmd == 0) cmd = "/usr/bin/pager";
		if (do_msg(L_INFORMATIONAL)) {
			fprintf(stderr, _("%s: using `%s' as pager\n"),
				get_progname(), cmd);
		}
		// popen does not set errno if memory allocation fails
		errno = ENOMEM;
		FILE * p = popen(cmd, "w");
		if (p != 0) {
			t = p;
			pipe = true;
		} else if (do_msg(L_IMPORTANT)) {
			fprintf(stderr, _("%s: popen failed for %s: %s\n"),
				get_progname(), cmd, strerror(errno));
		}
	}

	f = fopen (fname, "r");
	if (f == 0) {
		message(L_FATAL, COPYING, "%s", strerror (errno));
		return 0;
	}

	while ( ( c = getc (f)) != EOF) putc(c, t);

	if (ferror (f)) {
		message(L_FATAL, COPYING, "%s", strerror(errno));
		rv = 0;
	}

	fclose (f);

	if (pipe) pclose(t);

	return rv;
}

