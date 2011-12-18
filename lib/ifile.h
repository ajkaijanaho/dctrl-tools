/*  dctrl-tools - Debian control file inspection tools
    Copyright © 2004, 2011 Antti-Juhani Kaijanaho

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

#ifndef IFILE_H
#define IFILE_H

#include <stdbool.h>

struct ifile {
	enum ifile_mode { m_error, m_read, m_exec } mode;
	char const * s;
};

extern char const * const ifile_modes[];

// returns fd
int open_ifile(struct ifile f);

// must be used on ifile-opened fd's
void close_ifile(struct ifile f, int fd);

// check if a safe file to read from
bool chk_ifile(struct ifile fname, int fd);

#endif /* IFILE_H */
