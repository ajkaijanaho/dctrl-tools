/*  dctrl-tools - Debian control file inspection tools
    Copyright © 2005 Antti-Juhani Kaijanaho

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

// Marked portions of this file are covered by the following
// copyright:
/*
 * libdpkg - Debian packaging suite library routines
 * vercmp.c - comparison of version numbers
 *
 * Copyright © 1995 Ian Jackson <ian@chiark.greenend.org.uk>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef VERSION_H
#define VERSION_H

#include <stdbool.h>
#include <stddef.h>

struct versionrevision {  // from dpkg
  unsigned long epoch;
  char *version;
  char *revision;
};  

// from dpkg
int versioncompare(const struct versionrevision *version,
		   const struct versionrevision *refversion);

/* warning: modifies ver by adding nul chars; return pointers point
 * inside ver. */
bool parse_version(struct versionrevision *rv, char *ver, size_t len);

#endif /* VERSION_H */
