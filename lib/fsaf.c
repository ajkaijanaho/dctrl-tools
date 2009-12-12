/*  dctrl-tools - Debian control file inspection tools
    Copyright © 2003, 2005 Antti-Juhani Kaijanaho

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
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "align.h"
#include "fsaf.h"
#include "msg.h"

#if !defined(_POSIX_MAPPED_FILES) || _POSIX_MAPPED_FILES == -1
#  warning "No mmap support detected."
#endif

#define READAHEAD 1

bool fsaf_mmap;

FSAF * fsaf_fdopen(int fd, char const *fname)
{
	FSAF * rv = malloc(sizeof *rv);
	if (rv == 0) { errno = ENOMEM; return 0; }

	rv->fname = strdup(fname);
	if (rv->fname == NULL) { errno = ENOMEM; free(rv); return 0; }
	rv->fd = fd;
	rv->eof_mark = (size_t)(-1);
	rv->buf = 0;
	rv->buf_capacity = 0;
	rv->buf_offset = 0;
	rv->buf_size = 0;
	rv->invalid_mark = 0;
#ifdef _POSIX_MAPPED_FILES
	rv->mapped = false;
	rv->topread = 0;
#endif

	struct stat stat;
        int res = fstat(fd, &stat);
	if (res == -1) goto fail;

	if (S_ISREG(stat.st_mode)) {
		rv->eof_mark = stat.st_size;
#ifdef _POSIX_MAPPED_FILES
		if (fsaf_mmap) {
			/* try mmapping, it is a regular file */
			size_t eof_pagebound = pg_align(rv->eof_mark, true);
			char * buf = mmap(0, eof_pagebound, PROT_READ,
					  MAP_SHARED, fd, 0);
			if (buf != MAP_FAILED) {
				debug_message("mmapping", 0);
				rv->mapped = 1;
				rv->buf = buf;
				rv->buf_capacity = eof_pagebound;
				rv->buf_offset = 0;
				rv->buf_size = rv->eof_mark;
				//madvise(rv->buf, rv->buf_capacity,
				//	MADV_SEQUENTIAL);
			}
		}
#endif
	}

#ifdef _POSIX_MAPPED_FILES
	if (!rv->mapped)
#endif
	{
		rv->buf_capacity = 65536;
		rv->buf = malloc(rv->buf_capacity);
		if (rv->buf == 0) rv->buf_capacity = 0;
	}

	return rv;
fail:
	free(rv);
	return 0;
}

void fsaf_close(FSAF * fp)
{
	assert(fp != 0);

#if _POSIX_MAPPED_FILES
	if (fp->mapped) {
		munmap(fp->buf, fp->buf_capacity);
		free(fp);
		return;
	}
#endif
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

static void slurp(FSAF * fp, size_t len)
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

//static inline
struct fsaf_read_rv fsaf_read(FSAF * fp, size_t offset, size_t len)
{
	struct fsaf_read_rv rv;

        /* Reading nothing - since offset can be bogus in this
         * situation, this could foul up our assumptions later, so
         * return already here. */
        if (len == 0) {
                rv.b = "";
                rv.len = 0;
                return rv;
        }

	/* Make sure we don't read past the EOF mark.  */
	if (offset + len > fp->eof_mark) len = fp->eof_mark - offset;

#if _POSIX_MAPPED_FILES
	if (fp->mapped) {
#if 0 /* madvise seems not to give a noteworthy efficiency boost */
		/* Notify the kernel that we will soon want more of
		 * the data, but do this only once for each page!  */ 
		if (offset + len >= fp->topread) {
			size_t base = align(offset - fp->buf_offset, 1);
			size_t rl = READAHEAD * pagesize;
			madvise(fp->buf + base, rl, MADV_WILLNEED);
			fp->topread += rl;
		}
#endif
	} else
#endif
	{
		/* Ensure that we have enough data in the buffer.
		 * This is only executed if we are not dealing with a
		 * mapped file. */
		assert(offset >= fp->buf_offset);
		if (offset - fp->buf_offset + len > fp->buf_size) {
			slurp(fp, offset - fp->buf_offset + len - fp->buf_size);
			if (offset - fp->buf_offset + len > fp->buf_size) {
				len = fp->buf_size - (offset - fp->buf_offset);
			}
		}
		
	}

	assert(offset - fp->buf_offset + len <= fp->buf_size);
	assert(offset + len <= fp->eof_mark);
	rv.b = fp->buf + (offset - fp->buf_offset);
	rv.len = len;
	return rv;
}

void fsaf_invalidate(FSAF * fp, size_t offset)
{
	assert(fp->eof_mark >= offset);
	if (fp->invalid_mark >= offset) return;

#ifdef _POSIX_MAPPED_FILES
	if (fp->mapped) {
		size_t old = pg_align(fp->eof_mark - fp->buf_offset, 0);
		size_t new = pg_align(offset - fp->buf_offset, 0);
		madvise(fp->buf + old, new - old, MADV_DONTNEED);
	}
#endif	

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
