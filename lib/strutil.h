/*  dctrl-tools - Debian control file inspection tools
    Copyright (C) 1999, 2003 Antti-Juhani Kaijanaho

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


#ifndef STRUTIL_H__
#define STRUTIL_H__

#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/* Parse an integer from the (possibly not null-terminated) string s
 * of length len, and place the result in *rvp.  If return value is
 * false, *rvp is untouched. */
bool str2intmax(intmax_t * rvp, char const * s, size_t len);

/* Return a pointer to first nonblank character in s.  */
const char * left_trimmed(const char * s);

/* Mutate s and trim whitespace off its right end.  */
void trim_right(char * s);

/* Remove a trailing newline, if any. */
void chomp(char *);

/* Chop off everything after and including the first comchar in line.
   This mutates line!  */
static inline void chop_comment(char * line, char comchar)
{
	char * comstart;

	comstart = strchr (line, comchar);

	if (comstart != 0)
		*comstart = 0;
}

#endif /* STRUTIL_H__ */
