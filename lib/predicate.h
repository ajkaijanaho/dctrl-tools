/*  dctrl-tools - Debian control file inspection tools
    Copyright © 2003, 2004, 2005 Antti-Juhani Kaijanaho

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

#ifndef PREDICATE_H
#define PREDICATE_H

#include "paragraph.h"

#define MAX_OPS 4096
#define MAX_ATOMS 4096

#define I_NOP  0
#define I_NEG  1 /* --not; 1-1 */
#define I_AND  2 /* --and; 2-1 */
#define I_OR   3 /* --or;  2-1 */
#define I_PUSH(n) (4+(n)) /* push result of nth atomic proposition */

struct atom;

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
	struct atom *atoms;
};

void init_predicate(struct predicate * p);

static inline
struct atom * get_current_atom(struct predicate * p)
{
	assert(p->num_atoms > 0);
	return &p->atoms[p->num_atoms-1];
}

void addinsn(struct predicate * p, int insn);

bool does_para_satisfy(struct predicate * p, para_t *);

bool check_predicate(struct predicate * p);

#endif /* PREDICATE_H */
