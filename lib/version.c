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

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Marked portions of this file are covered by the following
// copyright:
/*
 * libdpkg - Debian packaging suite library routines
 * vercmp.c - comparison of version numbers
 * utils.c - Helper functions for dpkg
 *
 * Copyright © 1995 Ian Jackson <ian@chiark.greenend.org.uk>
 * Copyright © 2001 Wichert Akkerman <wakkerma@debian.org>
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
 * You should have received a copy of the GNU General Public
 * License along with dpkg; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <string.h>
#include "version.h"

/* <<<<<<< originally from dpkg >>>>>>> */

/* Reimplementation of the standard ctype.h is* functions. Since gettext
 * has overloaded the meaning of LC_CTYPE we can't use that to force C
 * locale, so use these cis* functions instead.
 */
static int cisdigit(int c) {
        return (c>='0') && (c<='9');
}

static int cisalpha(int c) {
        return ((c>='a') && (c<='z')) || ((c>='A') && (c<='Z'));
}

/* assume ascii; warning: evaluates x multiple times! */
#define order(x) ((x) == '~' ? -1 \
		: cisdigit((x)) ? 0 \
		: !(x) ? 0 \
		: cisalpha((x)) ? (x) \
		: (x) + 256)

static int verrevcmp(const char *val, const char *ref) {
  if (!val) val= "";
  if (!ref) ref= "";

  while (*val || *ref) {
    int first_diff= 0;

    while ( (*val && !cisdigit(*val)) || (*ref && !cisdigit(*ref)) ) {
      int vc= order(*val), rc= order(*ref);
      if (vc != rc) return vc - rc;
      val++; ref++;
    }

    while ( *val == '0' ) val++;
    while ( *ref == '0' ) ref++;
    while (cisdigit(*val) && cisdigit(*ref)) {
      if (!first_diff) first_diff= *val - *ref;
      val++; ref++;
    }
    if (cisdigit(*val)) return 1;
    if (cisdigit(*ref)) return -1;
    if (first_diff) return first_diff;
  }
  return 0;
}

int versioncompare(const struct versionrevision *version,
		   const struct versionrevision *refversion) {
  int r;

  if (version->epoch > refversion->epoch) return 1;
  if (version->epoch < refversion->epoch) return -1;
  r= verrevcmp(version->version,refversion->version);  if (r) return r;
  return verrevcmp(version->revision,refversion->revision);
}

/* <<<<<<< END OF originally from dpkg >>>>>>> */

/* warning: modifies ver by adding nul chars; return pointers point
 * inside ver. */
bool parse_version(struct versionrevision *rv, char *ver, size_t len)
{
	rv->version = strchr(ver, ':');
	if (rv->version == NULL) {
		rv->version = ver;
	} else {
		*(rv->version++) = '\0';
	}
	rv->revision = strrchr(ver, '-');

	if (rv->revision == NULL) {
		rv->revision = ver + len;
	} else {
		*(rv->revision++) = '\0';
	}
	
	if (rv->version != ver) {
		rv->epoch = 0;
		for (char *p = ver; *p != '\0'; p++) {
			if (!('0' <= *p && *p <= '9')) return false;
			rv->epoch = rv->epoch * 10 + (*p - '0');
		}
	} else {
		rv->epoch = 0;
	}

	for (char *p = rv->version; *p != '\0'; p++) {
		if (('a' <= *p && *p <= 'z') ||
		    ('A' <= *p && *p <= 'Z') ||
		    ('0' <= *p && *p <= '9') ||
		    *p == '.' ||
		    *p == '-' ||
		    *p == '+' ||
		    *p == ':' ||
		    *p == '~') continue;
		return false;
	}

	for (char *p = rv->revision; *p != '\0'; p++) {
		if (('a' <= *p && *p <= 'z') ||
		    ('A' <= *p && *p <= 'Z') ||
		    ('0' <= *p && *p <= '9') ||
		    *p == '.' ||
		    *p == '+' ||
		    *p == '~') continue;
		return false;
	}

	return true;
}

