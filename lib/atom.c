/*  dctrl-tools - Debian control file inspection tools
    Copyright © 2011, 2012 Antti-Juhani Kaijanaho

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

#include <regex.h>
#include <stdlib.h>
#include "atom.h"
#include "msg.h"
#include "version.h"

#define RE_PKG_BEGIN	"(^| )"
#define RE_PKG_END	"([, \\(]|$)"

void atom_finish(struct atom * atom)
{
	char * regex_pat = NULL;
	int regex_patlen = atom->patlen + strlen(RE_PKG_BEGIN)
				+ strlen(RE_PKG_END) + 1;
	debug_message("predicate_finish_atom", 0);
	if (atom->field_name != 0) {
                char * repl = strchr(atom->field_name, ':');
                if (repl != NULL) {
                        *repl++ = '\0';
                        atom->repl_inx = fieldtrie_insert(repl)->inx;
                } else {
                        atom->repl_inx = -1;
                }
		atom->field_inx = fieldtrie_insert(atom->field_name)->inx;
	}

	if (atom->mode == M_REGEX || atom->mode == M_EREGEX) {
		regex_pat = calloc(1, regex_patlen);	/* rely on mem 0-ing */
		if (regex_pat == 0)  fatal_enomem(0);
		if (atom->whole_pkg)
			strncat(regex_pat, RE_PKG_BEGIN, strlen(RE_PKG_BEGIN));
		strncat(regex_pat, atom->pat, atom->patlen);
		if (atom->whole_pkg)
			strncat(regex_pat, RE_PKG_END, strlen(RE_PKG_END));
		debug_message("compiling:", 0);
		debug_message(regex_pat, 0);
		int rerr = regcomp(&atom->regex, regex_pat,
				   (atom->mode == M_EREGEX ? REG_EXTENDED : 0)
				   | REG_NOSUB
				   | (atom->ignore_case ? REG_ICASE : 0));
		free(regex_pat);
		if (rerr != 0) {
			char * s;
			s = get_regerror(rerr, &atom->regex);
			if (s == 0) fatal_enomem(0);
			message(L_FATAL, 0, "%s", s);
			free(s);
			fail();
		}
	}

}

static bool atom_field_verify(struct atom * atom, FSAF * fp,
                              size_t start, size_t end)
{
	size_t len = end - start;
	struct fsaf_read_rv r = fsaf_read(fp, start, len);
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
		message(L_IMPORTANT, 0, "%s", s);
		free(s);
		return false;
	}
	case M_VER_EQ:case M_VER_LT:case M_VER_LE:case M_VER_GT:case M_VER_GE:
		;
		char *pats = strndup(atom->pat, atom->patlen);
		char *cands = strndup(r.b, len);
		struct versionrevision pat, cand;
		if (!parse_version(&pat, pats, atom->patlen)) {
			free(pats);
			free(cands);
			return false;
		}
		if (!parse_version(&cand, cands, len)) {
			free(pats);
			free(cands);
			return false;
		}
		int res = versioncompare(&cand, &pat);
		free(pats);
		free(cands);
		switch (atom->mode) {
		case M_VER_EQ:
			return res == 0;
		case M_VER_LT:
			return res < 0;
		case M_VER_LE:
			return res <= 0;
		case M_VER_GT:
			return res > 0;
		case M_VER_GE:
			return res >= 0;
		default:
			assert(0);
		}
	}
	assert(0);
}

bool atom_verify(struct atom * at, para_t * par)
{
        FSAF * fp = par->common->fp;
	if (at->field_inx == (size_t)-1) {
		/* Take the full paragraph */
                return atom_field_verify(at, fp, par->start, par->end);
        }
        /* Test field(s) */
        struct field_data fds = find_field_wr(par, at->field_inx, at->repl_inx);
        for (struct field_datum * fd = fds.first; fd != NULL; fd = fd->next) {
                if (atom_field_verify(at, fp, fd->start, fd->end)) return true;
        }
        return false;
}
