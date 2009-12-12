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

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef ALIGN_H
#define ALIGN_H

#include <assert.h>
#include <stdbool.h>
#include <unistd.h>

#define MAX_ALIGN 8

static inline
size_t get_pagesize(void)
{
	static size_t pagesize = 0;
	if (pagesize == 0) {
		pagesize = (size_t) sysconf(_SC_PAGESIZE);
	}
	assert(pagesize != 0);
	return pagesize;
}

static inline
size_t align(size_t offset, size_t alignment, bool ceil)
{
	return (offset / alignment + (ceil && offset % alignment != 0))
		* alignment;
}

static inline
size_t pg_align(size_t offset, bool ceil)
{
	return align(offset, get_pagesize(), ceil);
}

#endif /* ALIGN_H */
