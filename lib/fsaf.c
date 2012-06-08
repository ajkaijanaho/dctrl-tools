/*  dctrl-tools - Debian control file inspection tools
    Copyright © 2003, 2005, 2012 Antti-Juhani Kaijanaho

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
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "align.h"
#include "fsaf.h"
#include "msg.h"

#define READAHEAD 1

FSAF * fsaf_fdopen(int fd, char const *fname)
{
	FSAF * rv = malloc(sizeof *rv);
	if (rv == 0) { errno = ENOMEM; return 0; }

	rv->fname = strdup(fname);
	if (rv->fname == NULL) { errno = ENOMEM; free(rv); return 0; }
	rv->fd = fd;
	rv->eof_mark = (size_t)(-1);
	rv->buf_offset = 0;
	rv->buf_size = 0;
	rv->invalid_mark = 0;

	struct stat stat;
        int res = fstat(fd, &stat);
	if (res == -1) {
                free(rv);
                return 0;
        }

	if (S_ISREG(stat.st_mode)) {
		rv->eof_mark = stat.st_size;
	}

        rv->buf_capacity = 65536;
        rv->buf = malloc(rv->buf_capacity);
        if (rv->buf == 0) rv->buf_capacity = 0;

	return rv;
}

void fsaf_close(FSAF * fp)
{
	assert(fp != 0);

	free(fp->fname);
	free(fp->buf);
	free(fp);
}


static void slide(FSAF *fp)
{
	assert(fp->invalid_mark >= fp->buf_offset);
	size_t delta = fp->invalid_mark - fp->buf_offset;
	assert(fp->buf_capacity > delta);
	if (delta == 0) return;
	memmove(fp->buf, fp->buf + delta, fp->buf_size - delta);
	fp->buf_offset += delta;
	fp->buf_size -= delta;
}

void fsaf_slurp(FSAF * fp, size_t len)
{
	assert(fp != 0);
	assert(len > 0);
	if (fp->invalid_mark - fp->buf_offset > fp->buf_size / 2) {
		slide(fp);
	}
	if (fp->buf_size + len > fp->buf_capacity) {
		size_t nc = fp->buf_capacity;
		if (nc == 0) nc = 256;
		while (nc < fp->buf_size + len) nc *= 2;
		char * nb = realloc(fp->buf, nc);
		if (nb != 0) {
			fp->buf = nb;
			fp->buf_capacity = nc;
		} else {
			slide(fp);
			if (fp->buf_size > fp->buf_capacity) {
				fatal_enomem(fp->fname);
			}
			if (fp->buf_size + len > fp->buf_capacity) {
				len = fp->buf_capacity - fp->buf_size;
			}
		}
	}
	assert(len > 0);
	assert(fp->buf_size + len <= fp->buf_capacity);

	ssize_t res;
	do {
//		res = read(fp->fd, fp->buf + fp->buf_size, len);
		res = read(fp->fd, fp->buf + fp->buf_size, fp->buf_capacity - fp->buf_size);
	} while (res == -1 && errno == EINTR);
	if (res == 0) {
		fp->eof_mark = fp->buf_offset + fp->buf_size;
	}
	fp->buf_size += res;
}


void fsaf_invalidate(FSAF * fp, size_t offset)
{
	assert(fp->eof_mark >= offset);
	if (fp->invalid_mark >= offset) return;

	fp->invalid_mark = offset;
}

#ifdef TESTMAIN
#include <stdio.h>
volatile char c;
int main(int argc, char * argv[])
{
	if (argc == 1) {
		set_loglevel(L_DEBUG);
		FSAF * fp = fsaf_fdopen(STDIN_FILENO);
		assert(fp != 0);
		size_t i = 0;
		while (i < fsaf_eof(fp)) {
			struct fsaf_read_rv rr = fsaf_read(fp, i, 1);
			//fwrite(rr.b, 1, rr.len, stdout);
			c = rr.b[0];
			i += rr.len;
		}
		fsaf_close(fp);
	} else {
		int ch;
		while ((ch = getchar()) != EOF) {
			c = ch;
		}
	}
	return 0;
}
#endif
