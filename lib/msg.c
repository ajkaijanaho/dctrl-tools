/*   grep-dctrl - grep Debian control files
     Copyright © 1999, 2004, 2008, 2012  Antti-Juhani Kaijanaho
  
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


#define MSG_C__

#include <stdbool.h>

#include <assert.h>
#include <string.h>
#include "msg.h"

#define PROGNAME_MAXLEN 64

bool errors = false;

int loglevel = L_IMPORTANT;
char progname [PROGNAME_MAXLEN];

struct str2int_avec_t {
  const char * str;
  int integer;
};

int
within_interval (int n, int a, int b)
{
  if (a > b)
    {
      int tmp;
      tmp = b;
      b = a;
      a = tmp;
    }

  assert (a <= b);

  return (a <= n && n <= b);
}

void
msg_set_progname (const char * pn)
{
  strncpy (progname, pn, PROGNAME_MAXLEN);
  progname [PROGNAME_MAXLEN - 1] = 0;
}

void
set_loglevel (int ll)
{
  assert (within_interval (loglevel, L_FATAL, L_DEBUG));

  loglevel = ll;
}

int
str2loglevel (const char * s)
{
  static struct str2int_avec_t avec [] = {
    { "fatal", L_FATAL },
    { "important", L_IMPORTANT },
    { "informational", L_INFORMATIONAL },
#if !defined(NDEBUG) && defined(ENABLE_L_DEBUG)
    { "debug", L_DEBUG },
#endif
    { 0, 0 } };

  int i;

  for (i = 0; avec [i].str != 0; i++)
    if (strcasecmp (avec [i].str, s) == 0)
      return avec[i].integer;
  return -1;
}

void
msg_primitive(const char *fname, int line, const char *fmt, va_list ap)
{
        if (fname == 0) {
                fprintf(stderr,  "%s: ", get_progname());
        } else if (line > 0) {
                fprintf(stderr, "%s: %s:%i: ", get_progname(), fname, line);
        } else {
                fprintf(stderr, "%s: %s: ", get_progname(), fname);
        }
        vfprintf(stderr, fmt, ap);
        fputs(".\n", stderr);
}
