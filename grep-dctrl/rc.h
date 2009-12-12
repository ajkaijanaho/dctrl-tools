/*  dctrl-tools - Debian control file inspection tools
    Copyright © 1999, 2003 Antti-Juhani Kaijanaho

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

#ifndef RC_H__
#define RC_H__

#include "ifile.h"

extern char const * const ifile_modes[];

/* Search for exename (only basename, though) in rcfname (or if it is
   null, in default rc files) and return the found corresponding
   default input file name.  The mode will be m_error, if not found or
   other error occurred, m_read, if it is intended to be interpreted
   as a file name, and m_exec, if the string is to be interpreted as a
   shell command whose stdout should be used.  The returned pointer
   will be invalidated by the next call to this function.  */
struct ifile
find_ifile_by_exename (const char * exename, const char * rcfname);

#endif
