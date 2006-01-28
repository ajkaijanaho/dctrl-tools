/* getaline.c -- read arbitrarily long line from file
   (Originally from Lars Wirzenius' publib.)

   Copyright (c) 1994 Lars Wirzenius.  All rights reserved.
   Copyright (C) 2004 Antti-Juhani Kaijanaho.  All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
   OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "getaline.h"

// NULL indicates error, EOF is indicated by a string of length zero
char * getaline (FILE * f)
{
	char *buf;			/* buffer for line */
	size_t size;			/* size of buffer */
	size_t inc;			/* how much to enlarge buffer */
	size_t len;			/* # of chars  stored into buf
                                             before '\0' */
	char *p;
	const size_t thres = 128;	/* initial buffer size (most
					   lines should fit into this
					   size, so think of this as
					   the "long line
					   threshold").  */
	const size_t mucho = 128;	/* if there is at least this
					   much wasted space when the
					   whole buffer has been read,
					   try to reclaim it.  Don't
					   make this too small, else
					   there is too much time
					   wasted trying to reclaim a
					   couple of bytes.  */
	const size_t mininc = 64;	/* minimum number of bytes by
					   which to increase the
					   allocated memory */

	len = 0;
	size = thres;
	buf = malloc (size);
	if (buf == NULL) return NULL;

	buf[0] = '\0';

	while (fgets (buf + len, size - len, f) != NULL) {
		len += strlen(buf + len);
		if (len > 0 && buf[len - 1] == '\n')
			break;			/* the whole line has
						   been read */

		for (inc = size, p = NULL; inc > mininc; inc /= 2)
			if ((p = realloc (buf, size + inc)) != NULL)
				break;

		if (p == NULL) {
			free (buf);
			return NULL;		/* couldn't get more memory */
		}

		size += inc;
		buf = p;
	}

	if (len == 0 && ferror(f)) {
		free (buf);
		return NULL;		/* nothing read (eof or error) */
	}

#if 0
	if (buf[len - 1] == '\n')	/* remove newline, if there */
		buf[--len] = '\0';
#endif

	if (size - len > mucho)	{ /* a plenitude of unused memory? */
		p = realloc (buf, len + 1);

		if (p != NULL) {
			buf = p;
			size = len + 1;
		}
	}

	return buf;
}

