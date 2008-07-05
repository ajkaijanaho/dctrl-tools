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

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PARA_BUNDLE_H
#define PARA_BUNDLE_H

#include "fieldtrie.h"
#include "fsaf.h"
#include "ifile.h"
#include "paragraph.h"
#include "para_pool.h"

struct srcfile {
	struct ifile ifi;
	int fd;
	FSAF * fs;
	para_parser_t pp;
	struct srcfile * next;
};

struct para_bundle {
	para_pool_t pool;
	size_t num_paras;
	size_t max_num;
	para_t ** paras;
	struct srcfile * files;
};

static inline
void bundle_init(struct para_bundle * pb)
{
	para_pool_init(&pb->pool);
	pb->num_paras = 0;
	pb->max_num = 0;
	pb->paras = 0;
	pb->files = 0;
}

void bundle_slurp(struct para_bundle *, struct ifile);

static inline
size_t bundle_size(struct para_bundle * pb)
{
	assert(pb != 0);
	return pb->num_paras;
}

static inline
para_t ** bundle_vec(struct para_bundle * pb) 
{
	assert(pb != 0);
	return pb->paras;
}

#endif /* PARA_BUNDLE_H */
