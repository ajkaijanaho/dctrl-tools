/*  dctrl-tools - Debian control file inspection tools
    Copyright © 2003, 2004, 2005, 2007 Antti-Juhani Kaijanaho

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
#include <ctype.h>
#include <string.h>
#include "fieldtrie.h"
#include "msg.h"

struct field_bucket {
	struct field_attr attr;
	struct field_bucket * next;
};

struct fieldtrie_private {
	size_t nextfree;
	/* A one-level trie listing all field names occurring in the
	 * atomic predicates. */
	struct field_bucket * fields[UCHAR_MAX];
        /* An array mapping each field inx to its attributes */
        struct field_attr * field_map[65536];
};

static struct fieldtrie_private trie;

void fieldtrie_init(void)
{
	for (size_t i = 0; i < UCHAR_MAX; i++) {
		trie.fields[i] = 0;
	}
	trie.nextfree = 0;
}

struct field_attr *fieldtrie_insert_n(char const * s, size_t slen)
{
	struct field_attr *l_attr = fieldtrie_lookup(s, slen);
	if (l_attr != 0) return l_attr;
	struct field_bucket * b = malloc(sizeof *b);
	if (b == 0) fatal_enomem(0);
	char *name = malloc(slen+1);
	if (name == 0) fatal_enomem(0);
	strncpy(name, s, slen);
        (name)[slen] = '\0';
        *(char**)&b->attr.name = name;
	*(size_t*)&b->attr.namelen = slen;
        assert(trie.nextfree < sizeof trie.field_map / sizeof *trie.field_map);
	*(size_t*)&b->attr.inx = trie.nextfree++;
        b->attr.is_show_field = 0;
        b->attr.backup_field = (size_t)-1;
	unsigned char c = tolower((unsigned char)(b->attr.name[0]));
	b->next = trie.fields[c];
	trie.fields[c] = b;
        trie.field_map[b->attr.inx] = &b->attr;
	return &b->attr;
}

struct field_attr *fieldtrie_insert(char const * s)
{
        return fieldtrie_insert_n(s, strlen(s));
}

struct field_attr *fieldtrie_lookup(char const * s, size_t n)
{
	for (struct field_bucket * b = trie.fields[tolower((unsigned char)s[0])];
	     b != 0;
	     b = b->next) {
		if (n == b->attr.namelen &&
		    strncasecmp(s, b->attr.name, n) == 0) return &b->attr;
	}
	return NULL;
}

size_t fieldtrie_count(void)
{
	return trie.nextfree;
}

struct field_attr *fieldtrie_get(size_t inx)
{
        assert(inx < sizeof trie.field_map / sizeof *trie.field_map);
        return trie.field_map[inx];
}

#if 0
void fieldtrie_clear(void)
{
	for (size_t i = 0; i < UCHAR_MAX; i++) {
		struct field_bucket * b = trie.fields[i];
		while (b != 0) {
			struct field_bucket * bn = b->next;
			free(b);
			b = bn;
		}
		trie->fields[i] = 0;
	}
	trie->nextfree = 0;
}
#endif
