/*  dctrl-tools - Debian control file inspection tools
    Copyright (C) 2003 Antti-Juhani Kaijanaho

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

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include "fieldtrie.h"
#include "msg.h"

struct field_bucket {
	char const * name;
	size_t namelen;
	struct field_attr attr;
	struct field_bucket * next;
};

struct fieldtrie_private {
	size_t nextfree;
	/* A one-level trie listing all field names occurring in the
	 * atomic predicates. */
	struct field_bucket * fields[UCHAR_MAX];
};

static struct fieldtrie_private trie;

void fieldtrie_init(void)
{
	for (size_t i = 0; i < UCHAR_MAX; i++) {
		trie.fields[i] = 0;
	}
	trie.nextfree = 0;
}

size_t fieldtrie_insert(char const * s)
{
	size_t slen = strlen(s);
	struct field_attr l_attr = fieldtrie_lookup(s, slen);
	if (l_attr.valid) return l_attr.inx;
	struct field_bucket * b = malloc(sizeof *b);
	if (b == 0) fatal_enomem(0);
	b->name = malloc(slen+1);
	if (b->name == 0) fatal_enomem(0);
	strcpy((char*)b->name, s);
	b->namelen = slen;
	b->attr.inx = trie.nextfree++;
	b->attr.valid = true;
	unsigned char c = tolower((unsigned char)(b->name[0]));
	b->next = trie.fields[c];
	trie.fields[c] = b;
	return b->attr.inx;
}

struct field_attr fieldtrie_lookup(char const * s, size_t n)
{
	for (struct field_bucket * b = trie.fields[tolower((unsigned char)s[0])];
	     b != 0;
	     b = b->next) {
		if (n == b->namelen &&
		    strncasecmp(s, b->name, n) == 0) return b->attr;
	}
	return (struct field_attr){ .valid = false };
}

size_t fieldtrie_count(void)
{
	return trie.nextfree;
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
