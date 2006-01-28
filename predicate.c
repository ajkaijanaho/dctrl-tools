/*  dctrl-tools - Debian control file inspection tools
    Copyright (C) 2003, 2004 Antti-Juhani Kaijanaho

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

#include <ctype.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include "fsaf.h"
#include "msg.h"
#include "util.h"
#include "predicate.h"

void init_predicate(struct predicate * p)
{
	p->num_atoms = 0;
	p->proglen = 0;
	fieldtrie_init(&p->trie);
}

void addinsn(struct predicate * p, int insn)
{
	if (insn == I_NOP) return;
	if (p->proglen >= MAX_OPS) {
		message(L_FATAL, _("predicate is too complex"), 0);
		fail();
	}
	p->program[p->proglen++] = insn;
}

void predicate_finish_atom(struct predicate * p)
{
	struct atom * atom =  get_current_atom(p);
	debug_message("predicate_finish_atom", 0);
	if (atom->field_name != 0) {
		atom->field_inx = fieldtrie_insert(&p->trie, atom->field_name);
	}

	if (atom->mode == M_REGEX || atom->mode == M_EREGEX) {
		debug_message("compiling:", 0);
		debug_message(atom->pat, 0);
		int rerr = regcomp(&atom->regex, atom->pat,
				   (atom->mode == M_EREGEX ? REG_EXTENDED : 0)
				   | REG_NOSUB
				   | (atom->ignore_case ? REG_ICASE : 0));
		if (rerr != 0) {
			char * s;
			s = get_regerror(rerr, &atom->regex);
			if (s == 0) fatal_enomem(0);
			message(L_FATAL, s, 0);
			free(s);
			fail();
		}
	}
}

static bool verify_atom(struct atom * atom, para_t * para)
{
	size_t start, end;
	if (atom->field_inx == -1) {
		/* Take the full paragraph */
		start = para->start;
		end = para->end;
	} else {
		/* Take the field */
		struct field_data * fd = &para->fields[atom->field_inx];
		start = fd->start;
		end = fd->end;
	}
	size_t len = end - start;
	struct fsaf_read_rv r = fsaf_read(para->fp, start, len);
	assert(r.len == len);
	switch (atom->mode) {
	case M_EXACT:
		if (len != atom->patlen) return false;
		if (atom->ignore_case) {
			return strncasecmp(atom->pat, r.b, len) == 0;
		} else {
			return strncmp(atom->pat, r.b, len) == 0;
		}
	case M_SUBSTR: {
#if 0
		if (atom->ignore_case) {
			return strncasestr(r.b, atom->pat, len);
		} else {
			return strnstr(r.b, atom->pat, len);
		}
#else
		bool rv;
		char * s = strndup(r.b, len);
		if (s == 0) fatal_enomem(0);
		if (atom->ignore_case) {
			rv = strcasestr(s, atom->pat) != 0;
		} else {
			rv = strstr(s, atom->pat) != 0;
		}
		free(s);
		return rv;
#endif
	}
	case M_REGEX: case M_EREGEX: {
		char * s = strndup(r.b, len);
		if (s == 0) fatal_enomem(0);
		int regex_errcode = regexec(&atom->regex, s, 0, 0, 0);
		free(s);
		if (regex_errcode == 0 || regex_errcode == REG_NOMATCH) {
			return (regex_errcode == 0);
		}
		/* Error handling be here. */
		assert(regex_errcode != 0 && regex_errcode != REG_NOMATCH);
		s = get_regerror (regex_errcode, &atom->regex);
		if (s == 0) { enomem (0); return false; }
		message(L_IMPORTANT, s, 0);
		free(s);
		return false;
	}
	}
	assert(0);
}

bool check_predicate(struct predicate * p)
{
	size_t sp = 0;
	/* Run the program. */
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
	assert(para->trie == & p->trie);

	bool sat_atom[MAX_ATOMS];
	bool stack[MAX_OPS];
	size_t sp = 0;

	/* Verify atoms. */
	for (size_t i = 0; i < p->num_atoms; i++) {
		sat_atom[i] = verify_atom(&p->atoms[i], para);
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
