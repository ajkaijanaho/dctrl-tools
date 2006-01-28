/*  dctrl-tools - Debian control file inspection tools
    Copyright (C) 2003 Antti-Juhani Kaijanaho

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

#ifndef PREDICATE_H
#define PREDICATE_H

#include <assert.h>
#include <regex.h>
#include "fieldtrie.h"
#include "paragraph.h"

#define MAX_OPS 4096
#define MAX_ATOMS 4096

#define I_NOP 0
#define I_NEG 1 /* --not; 1-1 */
#define I_AND 2 /* --and; 2-1 */
#define I_OR  3 /* --or;  2-1 */
#define I_PUSH(n) (4+(n)) /* push result of nth atomic proposition */

/* An atomic predicate. */
struct atom {
	/* The name of field to which matching is limited.  Empty
	 * field_name specifies the whole paragraph (in which case
	 * field_inx is -1. */
	char const * field_name; size_t field_inx;
	/* Matching mode */
	enum { M_SUBSTR, /* substring matching */
	       M_REGEX, /* POSIX regular expression match */
	       M_EREGEX, /* POSIX extended regular expression matching */
	       M_EXACT /* exact match */
	       } mode;
	/* Flag: should matching ignore case */
	unsigned ignore_case;
	/* The pattern as given on the command line; interpretation
	 * depends on matching mode. */
	char const * pat; size_t patlen;
	/* A compiled version of pat; valid only when mode is M_REGEX
	 * or M_EREGEX.  */
	regex_t regex;
};

/* A predicate is represented as a set of atomic predicates and a
 * program - a sequence of stack-based "bytecode" instructions - that
 * specifies the structure of the combined predicate.  */
struct predicate {
	/* Number of atomic predicates.  */
	size_t num_atoms;
	/* Length of the program */
	size_t proglen;
	/* The program */
	int program[MAX_OPS];
	/* The atomic predicates */
	struct atom atoms[MAX_ATOMS];
	fieldtrie_t trie;
};

void init_predicate(struct predicate * p);

static inline
struct atom * get_current_atom(struct predicate * p)
{
	assert(p->num_atoms > 0);
	return &p->atoms[p->num_atoms-1];
}

void predicate_finish_atom(struct predicate *);

void addinsn(struct predicate * p, int insn);

bool does_para_satisfy(struct predicate * p, para_t *);

#endif /* PREDICATE_H */
