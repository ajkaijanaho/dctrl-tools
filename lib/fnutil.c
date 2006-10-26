/* fnutil.c - file name utilities
   (Originally composed from several files in Lars Wirzenius' publib.)

   Copyright (c) 1994 Lars Wirzenius.  All rights reserved.
   Copyright (C) 2004 Antti-Juhani Kaijanaho.  All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
   OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

#include <assert.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef USERNAME_MAX
#define USERNAME_MAX 9
#endif

char *fnbase(const char *fname)
{
	char *base;

	assert(fname != NULL);
	base = strrchr(fname, '/');
	if (base == NULL)
		return (char *) fname;
	return base+1;
}

char * fnqualify(char const * path)
{
	size_t len, size;
	struct passwd *pwd;
	const char *p;

	assert(path != NULL);

	/* Is it qualified already? */
	if (path[0] == '/') {
		return strdup(path);
	}

	/* Do we just need to prepend the current directory? */
	if (path[0] != '~') {
		char * cwd = get_current_dir_name();
		if (cwd == 0) return 0;
		len = strlen(cwd);
		size = len + 1 + strlen(path) + 1; /* +2 for '/' and '\0' */
		char * res = malloc(size);
		if (res == 0) {
			free(cwd);
			return 0;
		}
		sprintf(res, "%s/%s", cwd, path);
		return res;
	}

	/* We need to do tilde substitution, get the password entry (which
           includes the name of the home directory) */
	if (path[1] == '\0' || path[1] == '/') {
		pwd = getpwuid(getuid());
		if (path[1] == '\0')
			p = path + 1;
		else
			p = path + 2;
	} else {

		p = strchr(path, '/');
		if (p == NULL)
			p = strchr(path, '\0');
		size = (size_t) (p-path);
		char * username = malloc(size);
		if (username == 0) {
			errno = ENOMEM;
			return 0;
		}
		memcpy(username, path+1, size);
		username[size-1] = '\0';

		pwd = getpwnam(username);
		if (*p == '/')
			++p;
		free(username);
	}
	if (pwd == NULL) {
		errno = ENOENT;
		return 0;
	}


	/* Now we have all the necessary information, build the result */
	size = strlen(pwd->pw_dir) + 1 + strlen(p) + 1;
	char * result = malloc(size);
	if (result == 0) return 0;
	sprintf(result, "%s/%s", pwd->pw_dir, p);
	return result;
}
