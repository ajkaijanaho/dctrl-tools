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

struct atom;
struct predicate;

struct predicate *predicate_AND(struct predicate *, struct predicate *);
struct predicate *predicate_OR(struct predicate *, struct predicate *);
struct predicate *predicate_NOT(struct predicate *);
struct predicate *predicate_ATOM(struct atom *);

bool does_para_satisfy(struct predicate * p, para_t *);

bool check_predicate(struct predicate * p);

void predicate_print(struct predicate *p);

#endif /* PREDICATE_H */
