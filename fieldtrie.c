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

void fieldtrie_init(fieldtrie_t * trie)
{
	assert(trie != 0);
	for (size_t i = 0; i < UCHAR_MAX; i++) {
		trie->fields[i] = 0;
	}
	trie->nextfree = 0;
}

size_t fieldtrie_insert(fieldtrie_t * trie, char const * s)
{
	size_t slen = strlen(s);
	size_t r = fieldtrie_lookup(trie, s, slen);
	if (r != -1) return r;
	struct field_bucket * b = malloc(sizeof *b);
	if (b == 0) fatal_enomem(0);
	b->name = malloc(slen+1);
	if (b->name == 0) fatal_enomem(0);
	strcpy((char*)b->name, s);
	b->namelen = slen;
	b->inx = trie->nextfree++;
	unsigned char c = tolower((unsigned char)(b->name[0]));
	b->next = trie->fields[c];
	trie->fields[c] = b;
	return b->inx;
}

size_t fieldtrie_lookup(fieldtrie_t * trie, char const * s, size_t n)
{
	for (struct field_bucket * b = trie->fields[tolower((unsigned char)s[0])];
	     b != 0;
	     b = b->next) {
		if (n == b->namelen &&
		    strncasecmp(s, b->name, n) == 0) return b->inx;
	}
	return (size_t)(-1);
}

void fieldtrie_clear(fieldtrie_t * trie)
{
	for (size_t i = 0; i < UCHAR_MAX; i++) {
		struct field_bucket * b = trie->fields[i];
		while (b != 0) {
			struct field_bucket * bn = b->next;
			free(b);
			b = bn;
		}
		trie->fields[i] = 0;
	}
	trie->nextfree = 0;
}
