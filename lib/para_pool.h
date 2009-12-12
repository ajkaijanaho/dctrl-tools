/*  dctrl-tools - Debian control file inspection tools
    Copyright © 2004 Antti-Juhani Kaijanaho

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

#ifndef PARA_POOL_H
#define PARA_POOL_H

#include <stdint.h>
#include <stdlib.h>
#include "align.h"
#include "msg.h"
#include "paragraph.h"

#define NUM_PAGES 1

struct para_pages {
	size_t max;
	size_t nap; // next allocation "pointer" (offset from base of this)
	struct para_pages * next;
};

struct para_pool_private {
	struct para_pages * curr_pages;
};

typedef struct para_pool_private para_pool_t;

static inline
void para_pool_init(para_pool_t * pp)
{
	pp->curr_pages = 0;
}

void para_pool_fini(para_pool_t *); // deallocates everything allocated as well

/*
static inline
bool para_pool_more(para_pool_t * pp)
{
	size_t sz = NUM_PAGES * get_pagesize();
	struct para_pages * np = malloc(sz);
	assert(sizeof *np < sz); 
	if (np == 0) return false;
	np->next = pp->curr_pages;
	pp->curr_pages = np;
	np->nap = align(sizeof *np, MAX_ALIGN, true);
	np->max = sz;
	return true;
}
*/

// calls para_init
static inline
para_t * new_para(para_pool_t * ppo, para_parser_t * ppa)
{
	para_t * rv = 0;
/*
	struct para_pages * cp = ppo->curr_pages;
	if (cp == 0 || cp->nap + sizeof *rv >= cp->max) {
		bool success = para_pool_more(ppo);
		if (!success) return 0;
		cp = ppo->curr_pages;
	}
	assert(cp != 0);
	assert(cp->nap + sizeof *rv < cp->max);
	debug("cp->nap = %zi", cp->nap);
	rv = (para_t *) cp + cp->nap;
	assert((intptr_t)rv % __alignof__(*rv) == 0);
	cp->nap = align(cp->nap + sizeof *rv, MAX_ALIGN, true);
*/
	rv = malloc(sizeof *rv);
	if (rv == 0) return 0;
	para_init(ppa, rv);
	return rv;
}



#endif /* PARA_POOL_H */
