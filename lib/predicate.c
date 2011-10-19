/*  dctrl-tools - Debian control file inspection tools
    Copyright © 2003, 2004, 2008, 2010, 2011 Antti-Juhani Kaijanaho

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
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include "atom.h"
#include "fsaf.h"
#include "msg.h"
#include "util.h"
#include "predicate.h"
#include "strutil.h"
#include "version.h"

void init_predicate(struct predicate * p)
{
	p->num_atoms = 0;
	p->proglen = 0;
        p->atoms = malloc(MAX_ATOMS * sizeof p->atoms[0]);
        if (p->atoms == 0) enomem(0);
}

void addinsn(struct predicate * p, int insn)
{
	if (insn == I_NOP) return;
	if (p->proglen >= MAX_OPS) {
		message(L_FATAL, 0, _("predicate is too complex"));
		fail();
	}
	p->program[p->proglen++] = insn;
}


bool check_predicate(struct predicate * p)
{
	size_t sp = 0;
	/* Simulate the program. */
	for (size_t i = 0; i < p->proglen; i++) {
		switch (p->program[i]) {
		case I_NOP: break;
		case I_NEG:
			if (sp == 0) return false;
			break;
		case I_AND: case I_OR:
			if (sp < 2) return false;
			--sp;
			break;
		default:
			++sp;
		}
	}
	if (sp != 1) return false;
	return true;
}

bool does_para_satisfy(struct predicate * p, para_t * para)
{
	bool sat_atom[MAX_ATOMS];
	bool stack[MAX_OPS];
	size_t sp = 0;

	/* Verify atoms. */
	for (size_t i = 0; i < p->num_atoms; i++) {
		sat_atom[i] = atom_verify(&p->atoms[i], para);
	}

	/* Run the program. */
	for (size_t i = 0; i < p->proglen; i++) {
		switch (p->program[i]) {
		case I_NOP: break;
		case I_NEG:
			assert(sp >= 1);
			stack[sp-1] = !stack[sp-1];
			break;
		case I_AND:
			assert(sp >= 2);
			stack[sp-2] = stack[sp-2] && stack[sp-1];
			--sp;
			break;
		case I_OR:
			assert(sp >= 2);
			stack[sp-2] = stack[sp-2] || stack[sp-1];
			--sp;
			break;
		default:
		{
			int atom = p->program[i] - I_PUSH(0);
			assert(atom <= p->num_atoms);
			stack[sp] = sat_atom[atom];
			++sp;
		}
		}
	}
	assert(sp == 1);
	return stack[0];
}
