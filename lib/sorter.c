/*  dctrl-tools - Debian control file inspection tools
    Copyright (C) 2004, 2005 Antti-Juhani Kaijanaho

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

#include <stdlib.h>

#include "msg.h"
#include "sorter.h"
#include "version.h"

//void keys_fini(keys_t *);



static keys_t *keys;

int para_compare(keys_t *keys, const para_t *a, const para_t *b)
{
	int r = 0;
	for (size_t i = 0; i < keys->nks; i++) {
		char * af = get_field_as(a, keys->keys[i].field_inx);
		char * bf = get_field_as(b, keys->keys[i].field_inx);
		if (af == 0 || bf == 0) fatal_enomem(0);
		switch (keys->keys[i].type) {
		case FT_STRING:
			r = strcmp(af, bf);
			break;
		case FT_VERSION:
			;
			struct versionrevision ar, br;
			bool aok = parse_version(&ar, af, strlen(af));
			bool bok = parse_version(&br, bf, strlen(bf));
			if (!aok || !bok) {
				message(L_IMPORTANT,
					_("Parse error in field."), 0);
				free(af);
				free(bf);
				return aok ? (bok ? 0 : 1) : (bok ? -1 : 0);
			}
			r = versioncompare(&ar, &br);
			break;
		}
		debug("cmp: a = %s, b = %s, verdict is %d", af, bf, r);
		free(af);
		free(bf);
		if (keys->keys[i].reverse) r = -r;
		if (r != 0) break;
	}

	return r;
}

static
int compare(const void * av, const void * bv)
{
	debug("cmp: av = %p, bv = %p", av, bv);
	para_t ** a = (para_t **)av;
	para_t ** b = (para_t **)bv;

        return para_compare(keys, *a, *b);
}

void sort_bundle(keys_t * ks, struct para_bundle * pb)
{
	size_t num_paras = bundle_size(pb);
	para_t ** paras  = bundle_vec(pb);
	
	keys = ks;

	debug("sort_bundle: num_paras = %zd", num_paras);
	debug("sort_bundle: sizeof *paras = %zd", sizeof *paras);
	debug("sort_bundle: paras = %p, paras+num_paras*sizeof *paras = %p",
	      paras, paras+num_paras*sizeof *paras);

	qsort(paras, num_paras, sizeof *paras,  compare);
}
