/*  dctrl-tools - Debian control file inspection tools
    Copyright © 2012 Antti-Juhani Kaijanaho

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

#ifndef GUARD_LIB_STRLIST_H
#define GUARD_LIB_STRLIST_H

struct strlist;
struct strlist_iterator {
        struct strlist *sl;
        size_t i;
};
struct strlist_memento {
        struct strlist *sl;
        size_t n;
};

struct strlist *strlist_new(void);
_Bool strlist_append(struct strlist *, const char *);
_Bool strlist_is_empty(struct strlist *);
void strlist_free(struct strlist *);

struct strlist_memento strlist_save(struct strlist *);
void strlist_restore(struct strlist_memento);

struct strlist_iterator strlist_begin(struct strlist *);
_Bool strlist_iterator_at_end(struct strlist_iterator);
const char *strlist_iterator_get(struct strlist_iterator);
void strlist_iterator_next(struct strlist_iterator *);

#endif /* GUARD_LIB_STRLIST_H */
