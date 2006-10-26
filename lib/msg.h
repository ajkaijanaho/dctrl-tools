/*  dctrl-tools - Debian control file inspection tools
    Copyright (C) 1999 Antti-Juhani Kaijanaho

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

#ifndef MSG_H__
#define MSG_H__

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "i18n.h"

static inline
void fail(void) { exit(2); }

/* log levels */
#define L_FATAL 3
#define L_IMPORTANT 2
#define L_INFORMATIONAL 1
#define L_DEBUG 0

#if !defined(NDEBUG) && !defined(TEST_NODEBUG) && defined(ENABLE_L_DEBUG)
#  define INCLUDE_DEBUG_MSGS
#endif

inline static int
do_msg(int severity)
{
  extern int loglevel;

#if defined(TEST_NODEBUG)
  if (severity == L_DEBUG) {
	  message (L_FATAL, _("I'm broken - please report this bug."), 0, 0);
	  abort ();
  }
#endif
  
  return severity >= loglevel;
}

inline static char const *
get_progname(void)
{
	extern char progname [];
	return progname;
}

static inline
void record_error(void)
{
	extern bool errors;
	errors = 1;
}

inline static void
line_message (int severity, const char * s, const char * fname, int line)
{
	if (do_msg(severity)) {
      
		if (fname == 0) {
			fprintf (stderr,  "%s: %s.\n", get_progname(), s);
		} else {
			if (line > 0) {
				fprintf (stderr, "%s: %s:%i: %s.\n",
					 get_progname(), fname, line, s);
			} else {
				fprintf (stderr, "%s: %s: %s.\n", 
					 get_progname(), fname, s);
			}
		}
		if (severity >= L_IMPORTANT) record_error();
	}
}

inline static void
message (int severity, const char * s, const char * fname)
{
  line_message (severity, s, fname, 0);
}

#ifndef MSG_C__
#undef PROGNAME_MAXLEN
#endif

inline static void
debug_message (const char * s, const char * fname)
{
#ifdef INCLUDE_DEBUG_MSGS
  message (L_DEBUG, s, fname);
#endif
}

static void
debug(const char * s, ...) __attribute__((format(printf, 1, 2)));

inline static void
debug(const char * s, ...) 
{
#ifdef INCLUDE_DEBUG_MSGS
	if (do_msg(L_DEBUG)) {
		va_list va;
		va_start(va, s);
		fprintf(stderr, "%s: ", get_progname());
		vfprintf(stderr, s, va);
		fprintf(stderr, "\n");
		va_end(va);
	}
#endif
}

inline static void
errno_msg(int severity, char const * fname)
{
	message(severity, strerror(errno), fname);
}

#define enomem_msg _("cannot find enough memory")

inline static void
enomem (const char * fname)
{
  message (L_IMPORTANT, enomem_msg, fname);
}

inline static void
fatal_enomem (const char * fname)
{
  message(L_FATAL, enomem_msg, fname);
  fail();
}

#undef enomem_msg

/* Given a string that represents a log level, return the number that
   repsesents that loglevel.  A negative return value means that the
   string is not a valid log level.  */
int
str2loglevel (const char * s);

/* Set current log level to ll.  */
void
set_loglevel (int ll);

/* Set program name to pn. */
void
msg_set_progname (const char * pn);

/* Return true iff loglevels IMPORTANT or FATAL have been used.  */
static inline
bool errors_reported(void)
{
	extern bool errors;
	return errors;
}

#endif /* MSG_H__ */
