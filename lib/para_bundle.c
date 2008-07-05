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

#include "msg.h"
#include "para_bundle.h"

void bundle_slurp(struct para_bundle * pb, struct ifile ifi)
{
	int fd = open_ifile(ifi);
	if (fd == -1) {
		record_error();
		return;
	}
	
	FSAF * f = fsaf_fdopen(fd, ifi.s);
	if (f == 0) fatal_enomem(0);

	struct srcfile * sf = malloc(sizeof *sf);
	if (sf == 0) fatal_enomem(0);

	sf->ifi = ifi;
	sf->fd = fd;
	sf->fs = f;
	sf->next = pb->files;
	pb->files = sf;

	para_parser_init(&sf->pp, f, false, false, false);
	while (true) {
		if (pb->num_paras == pb->max_num) {
			size_t max_num = pb->max_num == 0 ? 256 :
				2 * pb->max_num;
			para_t ** paras = realloc(pb->paras,
						  max_num 
						  * sizeof *paras);
			if (paras == 0) fatal_enomem(0);
			pb->max_num = max_num;
			pb->paras = paras;
		}
		assert(pb->num_paras < pb->max_num);
		assert(pb->paras != 0);
		para_t * p = new_para(&pb->pool, &sf->pp);
		if (p == 0) fatal_enomem(0);
		para_parse_next(p);
		if (para_eof(&sf->pp)) break;
		debug("pb->num_paras = %zi", pb->num_paras);
		pb->paras[pb->num_paras++] = p;
	}

//      DO NOT CLOSE fsaf -- this makes the whole para_bundle useless!
//	fsaf_close(f);
	close_ifile(ifi, fd);
}
