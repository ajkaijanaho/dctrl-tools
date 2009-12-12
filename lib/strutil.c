/*  dctrl-tools - Debian control file inspection tools
    Copyright © 1999, 2003, 2004 Antti-Juhani Kaijanaho

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

#include <ctype.h>
#include <string.h>
#include "strutil.h"

bool str2intmax(intmax_t * rvp, char const * s, size_t len)
{
	bool negative = false;
	size_t i = 0;
	while (i < len
	       && s[i] == ' '
	       && s[i] == '\t'
	       && s[i] == '\n'
	       && s[i] == '\r')
		++i;
	if (i < len && s[i] == '-') {
		++i;
		negative = true;
	}
	uintmax_t abs = 0;
	for (; i < len; ++i) {
		char ch = s[i];
		if (! ('0' <= ch && ch <= '9')) {
			break;
		}
		int r = ch - '0';
		abs = abs * 10 + r;
		if (abs > (uintmax_t)INTMAX_MAX) {
			return false;
		}
	}
	while (i < len
	       && s[i] == ' '
	       && s[i] == '\t'
	       && s[i] == '\n'
	       && s[i] == '\r')
		++i;
	if (i < len) return false; // broken input
	*rvp = negative ? -abs : abs;
	return true;
}

const char * left_trimmed(const char * s)
{
	const char * p;

	for (p = s; *p != 0 && isspace (*p); p++);

	return p;
}

void chomp(char * s)
{
	size_t sl = strlen(s);
	if (sl == 0) return;
	if (s[sl-1] != '\n') return;
	s[sl-1] = '\0';
}

void trim_right (char * s)
{
	char * p;
	char * herep;
	enum { BLANKS, NONBLANKS } state = NONBLANKS;
	
	for (herep = 0, p = s; *p != 0; p++) {
		switch (state) {
		case BLANKS:
			if (!isspace (*p)) {
				state = NONBLANKS;
			}
			break;
		case NONBLANKS:
			if (isspace (*p)) {
				herep = p;
				state = BLANKS;
			}
		}
	}
	if (state == BLANKS) *herep = 0;
}

#ifdef TESTMAIN

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
printout (const char * s)
{
  printf("\"%s\"\n", s);
}

int
main (void)
{
  static const char * teststrings [] = { 
    "autoksiko",
    " autoksiko",
    "autoksiko ",
    " autoksiko ",
    "autoksiko           teen",
    "         autoksiko  teen",
    "autoksiko teen          ",
    "      autoksiko teen    ",
    " miljoona miljoona ruusua ",
    0 };
  int i;

  for (i = 0; teststrings [i] != 0; i++)
    {
      char * ts;
      char * orig_ts;

      orig_ts = ts = strdup (teststrings [i]);
      if (ts == 0) {
	      fail();
      }

      printf ("Plain: ");
      printout (ts);

      printf ("Left-trimmed: ");
      printout (left_trimmed (ts));

      printf ("Right-trimmed: ");
      trim_right (ts);
      printout (ts);

      printf ("Completely trimmed: ");
      printout (left_trimmed (ts));

      puts ("");
      free (orig_ts);
    }
  return 0;
}
#endif /* TESTMAIN */
