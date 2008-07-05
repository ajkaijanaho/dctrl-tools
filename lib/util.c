/*  dctrl-tools - Debian control file inspection tools
    Copyright © 2003, 2004 Antti-Juhani Kaijanaho

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

//#include <publib.h>
#include <stdlib.h>
#include <regex.h>
#include "util.h"

char * get_regerror (int errcode, regex_t *compiled)
{
  size_t length = regerror (errcode, compiled, NULL, 0);
  char * buffer = malloc (length);

  if (buffer == 0)
    return 0;

  (void) regerror (errcode, compiled, buffer, length);
  return buffer;
}

/*
char * fnqualify_xalloc(const char * fname)
{
	char * rv = 0;
	size_t rv_len = 0;
	int actual_len = 0;

	while (actual_len >= rv_len) {
		rv = xrealloc(rv, rv_len += 64);
		actual_len = fnqualify(rv, fname, rv_len);
	}

	return rv;
}
*/
