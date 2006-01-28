/*   grep-dctrl - grep Debian control files
     Copyright (C) 1999  Antti-Juhani Kaijanaho
  
     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 2 of the License, or
     (at your option) any later version.
  
     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details. 
  
     You should have received a copy of the GNU General Public License
     along with this program; see the file COPYING.  If not, write to
     the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
     Boston, MA 02111-1307, USA.
  
     The author can be reached via mail at (ISO 8859-1 charset for the city)
        Antti-Juhani Kaijanaho
        Helvintie 2 e as 9
        FIN-40500 JYVÄSKYLÄ
        FINLAND
        EUROPE
     and via electronic mail from
        gaia@iki.fi
     If you have a choice, use the email address; it is more likely to
     stay current.

*/


#define MSG_C__

#include <stdbool.h>

bool errors = false;

#include <assert.h>
#include <string.h>
#include "msg.h"

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
