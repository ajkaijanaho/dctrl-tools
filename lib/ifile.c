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

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "ifile.h"
#include "msg.h"

static int open_pipe(char const * s)
{
	int ps[2];
	int r = pipe(ps);
	if (r == -1) {
		errno_msg(L_IMPORTANT, 0);
		return -1;
	}
	pid_t pid = fork();
	switch (pid) {
	case -1:
		errno_msg(L_IMPORTANT, 0);
		close(ps[0]);
		close(ps[1]);
		return -1;
	case 0:
		// child
		close(STDIN_FILENO);
		r = dup2(ps[1], STDOUT_FILENO);
		if (r == -1) {
			fprintf(stderr, "%s (child): %s\n",
				get_progname(), strerror(errno));
			_exit(1);
		}
		close(ps[0]);
		close(ps[1]);
		execl("/bin/sh", "/bin/sh", "-c", s, (char*)0);
		fprintf(stderr, _("%s (child): failed to exec /bin/sh: %s\n"),
			get_progname(), strerror(errno));
		_exit(1);
	}
	// parent
	close(ps[1]);
	return ps[0];
}

int open_ifile(struct ifile f)
{
	switch (f.mode) {
	case m_read:
		if (strcmp(f.s, "-") == 0) {
			return STDIN_FILENO;
		}
		{
			int fd = open(f.s, O_RDONLY);
			if (fd == -1) {
				fprintf(stderr, "%s: %s: %s\n",
					get_progname(), f.s, strerror(errno));
				record_error();
				return -1;
			}
			return fd;
		}
	case m_exec:
		return open_pipe(f.s);
	case m_error:
		abort();
	}
	abort();
}

void close_ifile(struct ifile f, int fd)
{
	close(fd);
	if (f.mode == m_exec) {
		int status;
		int r = wait(&status);
		if (r == -1) {
			errno_msg(L_IMPORTANT, f.s);
			return;
		}
		if (WIFEXITED(status)) {
			if (WEXITSTATUS(status) == 0) return;
			fprintf(stderr, _("%s: command (%s) failed "
					  "(exit status %d)\n"),
				get_progname(),
				f.s,
				WEXITSTATUS(status));
			record_error();
			return;
		}
		if (WIFSIGNALED(status)) {
			fprintf(stderr, _("%s: command (%s) was killed "
					  "by signal %d\n"),
				get_progname(),
				f.s,
				WTERMSIG(status));
			record_error();
			return;
		}
	}
}

bool chk_ifile(struct ifile fname, int fd)
{
	struct stat stat;
	int r = fstat(fd, &stat);
	mode_t m = stat.st_mode;
	if (r == -1) {
		if (do_msg(L_IMPORTANT)) {
			fprintf(stderr, _("%s: %s: cannot stat: %s\n"),
				get_progname(), fname.s, strerror(errno));
		}
		record_error();
		close_ifile(fname, fd);
		return 0;
	}
	if (!(S_ISREG(m) || S_ISCHR(m) || S_ISFIFO(m))) {
		if (do_msg(L_IMPORTANT)) {
			fprintf(stderr, "%s: %s: %s\n",
				get_progname(), fname.s,
				S_ISDIR(m) ? _("is a directory, skipping") :
				S_ISBLK(m) ? _("is a block device, skipping") :
				S_ISLNK(m) ? _("internal error") :
				S_ISSOCK(m) ? _("is a socket, skipping") :
				_("unknown file type, skipping"));
		}
		record_error();
		close_ifile(fname, fd);
		return 0;
	}
	return 1;
}
