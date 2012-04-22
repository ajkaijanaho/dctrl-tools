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

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include "strlist.h"

struct strlist {
        const char **strs;
        size_t n;
        size_t maxn;
};

struct strlist *strlist_new(void)
{
        struct strlist *rv = malloc(sizeof *rv);
        if (rv == 0) return 0;
        rv->strs = 0;
        rv->n = 0;
        rv->maxn = 0;
        return rv;
}

bool strlist_append(struct strlist *sl, const char *s)
{
        if (sl->n + 1 >= sl->maxn) {
                size_t newmax = sl->maxn * 2;
                if (newmax == 0) newmax = 2;
                const char **newstrs = realloc(sl->strs,
                                               newmax * sizeof *newstrs);
                if (newstrs == 0) return false;
                sl->maxn = newmax;
                sl->strs = newstrs;
        }
        assert(sl->n < sl->maxn);
        sl->strs[sl->n++] = s;
        return true;
}
bool strlist_is_empty(struct strlist *sl)
{
        return sl->n == 0;
}

void strlist_free(struct strlist *sl)
{
        free(sl->strs);
        free(sl);
}

struct strlist_iterator strlist_begin(struct strlist *sl)
{
        struct strlist_iterator it = {
                .sl = sl,
                .i = 0
        };
        return it;
}
bool strlist_iterator_at_end(struct strlist_iterator it)
{
        return it.i >= it.sl->n;
}

const char *strlist_iterator_get(struct strlist_iterator it)
{
        return it.sl->strs[it.i];
}

void strlist_iterator_next(struct strlist_iterator * it)
{
        it->i++;
}

struct strlist_memento strlist_save(struct strlist *sl)
{
        struct strlist_memento rv = {
                .sl = sl,
                .n = sl->n
        };
        return rv;
}
void strlist_restore(struct strlist_memento mem)
{
        mem.sl->n = mem.n;
}

