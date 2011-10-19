/*  dctrl-tools - Debian control file inspection tools
    Copyright © 2011 Antti-Juhani Kaijanaho

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

#ifndef GUARD_LIB_ATOM_H
#define GUARD_LIB_ATOM_H

#include <regex.h>
#include "paragraph.h"
#include "util.h"

/* An atomic predicate. */
struct atom {
	/* The name of field to which matching is limited.  Empty
	 * field_name specifies the whole paragraph (in which case
	 * field_inx is -1. */
	char const * field_name; size_t field_inx;
        /* The index to the field whose value is to be used when this
         * field is empty. */
        size_t repl_inx;
	/* Matching mode */
	enum matching_mode {
		M_SUBSTR, /* substring matching */
		M_REGEX, /* POSIX regular expression match */
		M_EREGEX, /* POSIX extended regular expression matching */
		M_EXACT, /* exact string match */
#define M_FIRST_VERSION M_VER_EQ
		M_VER_EQ, /* numeric equality comparison */
		M_VER_LT, /* numeric < */
		M_VER_LE, /* numeric <= */
		M_VER_GT, /* numeric > */
		M_VER_GE, /* numeric >= */
#define M_LAST_VERSION M_VER_GE
	} mode;
	/* Flag: should matching ignore case */
	unsigned ignore_case;
	/* The pattern as given on the command line; interpretation
	 * depends on matching mode. Must be null-terminated and
	 * patlen must equal strlen(pat).  */
	char const * pat; size_t patlen;
	/* A compiled version of pat; valid only when mode is M_REGEX
	 * or M_EREGEX.  */
	regex_t regex;
	/* Flag: (extended) regex should match whole package names */
	unsigned whole_pkg;
};

void atom_finish(struct atom * atom);
bool atom_verify(struct atom * atom, para_t * para);

#endif /* GUARD_LIB_ATOM_H */
