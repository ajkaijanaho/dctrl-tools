.TH GREP-DCTRL 1 2012-04-22 "Debian Project" "Debian user's manual"
\" Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2011,
\"               2012
\"               Antti-Juhani Kaijanaho <ajk@debian.org>
\"      This program is free software; you can redistribute it and/or modify
\"      it under the terms of the GNU General Public License as published by
\"      the Free Software Foundation; either version 2 of the License, or
\"      (at your option) any later version.
\" 
\"      This program is distributed in the hope that it will be useful,
\"      but WITHOUT ANY WARRANTY; without even the implied warranty of
\"      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
\"      GNU General Public License for more details. 
\"  
\"      You should have received a copy of the GNU General Public License
\"      along with this program; see the file COPYING.  If not, write to
\"      the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
\"      Boston, MA 02111-1307, USA.
.SH NAME
grep\-dctrl, grep\-status, grep\-available, grep\-aptavail, grep\-debtags \- grep Debian control files
.SH SYNOPSIS
.I command
.BR --copying "|" -C " | " --help "|" -h " | " --version "|" -V 
.sp
.IR command " [" options "] " filter " [ " file "... ]"
.sp
where
.I command
is one of
.BR grep\-dctrl ,
.BR grep\-status ,
.BR grep\-available ,
.B grep\-aptavail 
and
.BR grep\-debtags .
.SH DESCRIPTION
The
.B grep\-dctrl
program can answer such questions as 
.IR "What is the Debian package foo?" , 
.IR "Which version of the Debian package bar is now current?" ,
.IR "Which Debian packages does John Doe maintain?" ,
.I  "Which Debian packages are somehow related to the Scheme"
.IR "programming language?" ,
and with some help,
.IR "Who maintain the essential packages of a Debian system?" ,
given a useful input file.
.PP
The programs
.BR grep\-available ,
.BR grep\-status ,
.B grep\-aptavail
and
.B grep\-debtags
are aliases of (actually, symbolic links to)
.BR grep\-dctrl .
These aliases use as their default input
the
.BR dpkg (1)
.I available
and
.I status
files, the
.B apt\-cache dumpavail
output and the
.B debtags dumpavail
output, respectively.
.PP
.B grep\-dctrl
is a specialised
.B grep
program that is meant for processing any file
which has the general format of a Debian package
.I control
file, as
described in the Debian Policy.  These include the
.B dpkg
.I available
file, the
.B dpkg
.I status
file, and the
.I Packages
files on a
distribution medium (such as a Debian CD-ROM or an FTP site carrying
Debian).
.PP
You must give a
.I filter
expression on the command line.  The
.I filter
defines which kind of paragraphs (aka package records) are output.  A
simple 
.I filter
is a search pattern along with any options that modify
it.  Possible modifiers are
.BR \-\-eregex ", " \-\-field ", " \-\-ignore\-case ", " \-\-regex
and
.BR \-\-exact\-match ,
along with their single-letter equivalents.  By
default, the search is a case-sensitive fixed substring match on each
paragraph (in other words, package record) in the input.  With
suitable modifiers, this can be changed: the search can be
case-insensitive and the pattern can be seen as an extended POSIX
regular expression. 
.PP
.IR Filter s
can be combined to form more complex
.IR filter s
using the connectives
.BR \-\-and ", " \-\-or " and " \-\-not .
Parentheses (which usually
need to be escaped for the shell) can be used for grouping.
.PP
By default, the full matching paragraphs are printed on the standard
output; specific fields can be selected for output with the
.B \-s
option.
.PP
After the 
.I filter
expression comes zero or more 
.I file
names.  The 
.I file
name
.B \-
is taken to mean the standard input stream.  The
.IR file s
are searched in order but separately; they are
.B not
concatenated together.  In other words, the end of a 
.I file
always implies the end of the current paragraph.
.PP
If no
.I file
names are specified, the program name is used to identify a default
input file.  The program names are matched with the base form of the
name of the current program (the 0'th command line argument, if you
will).
.SH OPTIONS
.SS Specifying the search pattern
.IP "\fB\-\-pattern=\fIpattern"
Specify a
.I pattern
to be searched. This switch is not generally needed, as the
.I pattern
can be given by itself. However,
.IR pattern s
that start
with a dash
.RB ( - )
must be given using this switch, so that they wouldn't
be mistaken for switches.
.SS Modifiers of simple filters
.IP "\fB\-F \fIfield\fR,\fIfield\fR, ... | \fB\-\-field=\fIfield\fR,\fIfield\fR, ..."
Restrict pattern matching to the 
.IR field s
given.  Multiple
.I field
names in one 
.B -F
option and multiple 
.B -F
options in one simple 
.I filter 
are
allowed. The search named by the filter will be performed
among all the
.IR field s
named, and as soon as any one of them matches, the
whole simple 
.I filter
is considered matching.
.IP
A 
.I field
specification can contain a colon
.RB ( : ).
In such a case, the part
up to the colon is taken as the name of the field to be searched in,
and the part after the colon is taken as the name of the field whose
content is to be used if the field to search in is empty.
.IP \fB\-P
Shorthand for
.BR \-FPackage .
.IP \fB\-S
Shorthand for
.BR \-FSource:Package .
.IP "\fB\-e\fR, \fB\-\-eregex"
Regard the pattern of the current simple filter as an extended
POSIX regular expression
.IP "\fB\-r\fR, \fB\-\-regex"
Regard the pattern of the current simple filter as a standard POSIX regular expression.
.IP "\fB\-i\fR, \fB\-\-ignore\-case"
Ignore case when looking for a match in the current simple filter.
.IP "\fB\-X\fR, \fB\-\-exact\-match"
Do an exact match (as opposed to a substring match) in the current
simple filter.
.IP "\fB\-w\fR, \fB\-\-whole\-pkg"
Do an extended regular expression match on whole package names,
assuming the syntax of inter-package relationship fields such as
.BR Depends , Recommends ", ..."
When this flag is given you should not worry
about sub-package names such as "libpcre3" also matching
"libpcre3-dev". This flag implies (and is incompatible with)
.BR \-e .
.IP "\fB\-\-eq"
Do an equality comparison under the Debian version number system.  If
the pattern or the field to be searched in is not a valid Debian
version number, the paragraph is regarded as not matching.  As a
special case, this is capable of comparing simple nonnegative integers
for equality.
.IP "\fB\-\-lt"
Do an strictly-less-than comparison under the Debian version number
system.  If the pattern or the field to be searched in is not a valid
Debian version number, the paragraph is regarded as not matching.  As
a special case, this is capable of comparing simple nonnegative
integers.
.IP "\fB\-\-le"
Do an less-than-or-equal comparison under the Debian version number
system.  If the pattern or the field to be searched in is not a valid
Debian version number, the paragraph is regarded as not matching.  As
a special case, this is capable of comparing simple nonnegative
integers.
.IP "\fB\-\-gt"
Do an strictly-greater-than comparison under the Debian version number
system.  If the pattern or the field to be searched in is not a valid
Debian version number, the paragraph is regarded as not matching.  As
a special case, this is capable of comparing simple nonnegative
integers.
.IP "\fB\-\-ge"
Do an greater-than-or-equal comparison under the Debian version number
system.  If the pattern or the field to be searched in is not a valid
Debian version number, the paragraph is regarded as not matching.  As
a special case, this is capable of comparing simple nonnegative
integers.
.SS Combining filters
.IP "\fB\-!\fR, \fB\-\-not\fR, \fB!"
Match if the following filter does 
.B not
match.
.IP "\fB\-o\fR, \fB\-\-or"
Match if either one or both of the preceding and following filters
matches.
.IP "\fB\-a\fR, \fB\-\-and"
Match if both the preceding and the following filter match.
.IP "\fB(\fR ... \fB)"
Parentheses can be used for grouping.  Note that they need to be
escaped for most shells.  Filter modifiers can be given before the
opening parentheses; they will be treated as if they had been repeated
for each simple filter inside the parentheses.
.SS Output format modifiers
.IP "\fB\-s \fIfield\fR,\fIfield\fR, ... | \fB\-\-show\-field=\fIfield\fR,\fIfield\fR, ..."
Show only the body of these
.IR field s
from the matching paragraphs.  The
.I field
names must not include any colons or commas.  Commas are used to
delimit
.I field
names in the argument to this option.  The
.IR field s
are shown in the order given here.  See also the option
.BR \-I .
Note that in the absence of the
.B \-\-ensure\--dctrl
option, if only one field is selected, no paragraph separator is output.
.IP "\fB\-I\fR, \fB\-\-invert\-show"
Invert the meaning of option
.BR \-s :
show only the fields that have
.B not
been named using a 
.B \-s
option.  As an artefact of the implementation,
the order of the fields in the original paragraph is not preserved.
.PP
A 
.I field
specification can contain a colon. In such a case, the part
up to the colon is taken as the name of the field to be shown, and the
part after the colon is taken as the name of the field whose content
is to be used if the field to be shown is empty.
.IP "\fB\-d"
Show only the first line of the
.B Description
field from the matching
paragraphs.  If no 
.B \-s
option is specified, this option also effects
.BR "\-s Description";
if there is a 
.B \-s
option but it does not include
the
.B Description
field name, one is appended to the option.  Thus the
.B Description
field's location in the output is determined by the 
.B \-s
option, if any, the last field being the default.
.IP "\fB\-n\fR, \fB\-\-no\-field\-names"
Suppress field names when showing specified fields, only their bodies
are shown.  Each field is printed in its original form without the
field name, the colon after it and any whitespace preceding the start
of the body.
.IP "\fB\-v\fR, \fB\-\-invert\-match"
Instead of showing all the paragraphs that match, show those paragraphs
that do
.B not
match.
.IP "\fB\-c\fR, \fB\-\-count"
Instead of showing the paragraphs that match (or, with 
.BR \-v ,
that don't
match), show the count of those paragraphs.
.IP "\fB\-q\fR, \fB\-\-quiet\fR, \fB\-\-silent"
Output nothing to the standard output stream.  Instead, exit
immediately after finding the first match.
.SS Miscellaneous
.IP "\fB\-\-ensure\-dctrl"
Ensure that the output is in dctrl format, specifically that there always
is an empty line separating paragraphs.  This option is not honored if 
the 
.B \-n
option has been selected, as that option deliberately requests a non-dctrl
format for the output.  In a future version, this option may be made the
default behaviour.
.IP "\fB\-\-compat"
Override any
.B \-\-ensure\-dctrl
option given earlier on the command line.
.IP "\fB\-\-ignore\-parse\-errors"
Ignore errors in parsing input.  A paragraph which cannot be parsed
is ignored in its entirety, and the next paragraph is assumed to start
after the first newline since the location of the error.
.IP "\fB\-\-debug\-optparse"
Show how the current command line has been parsed. 
.IP "\fB\-l \fIlevel\fR, \fB\-\-errorlevel=\fIlevel"
Set log level to
.IR level .
.I level
is one of 
.BR fatal ", " important ", " informational " and " debug ,
but the last may not be available,
depending on the compile-time options.  These categories are given
here in order; every message that is emitted when 
.B fatal
is in effect, will be emitted in the 
.B important
error level, and so on. The default is 
.BR important .
.IP "\fB\-V\fR, \fB\-\-version"
Print out version information.
.IP "\fB\-C\fR, \fB\-\-copying"
Print out the copyright license.  This produces much output; be sure
to redirect or pipe it somewhere (such as your favourite pager).
.IP "\fB\-h\fR, \fB\-\-help"
Print out a help summary.
.SH EXAMPLES
The almost simplest use of this program is to print out the status or
available record of a package.  In this respect,
.B grep\-dctrl
is like
.B "dpkg \-s"
or
.BR "dpkg \-\-print\-avail".
To print out the status record of the package "mixal", do
.nf
% grep\-status \-PX mixal
.fi
and to get its available record, use
.nf
% grep\-available \-PX mixal
.fi
In fact, you can ask for the record of the "mixal" package
from any Debian control file.  Say, you have the Debian 6.0
CD-ROM's
.I Packages
file in the current directory; now you
can do a
.nf
% grep\-dctrl \-PX mixal Packages
.fi
.PP
But
.B grep\-dctrl
can do more than just emulate
.BR dpkg .
It can more-or-less emulate
.BR apt\-cache !
That program has a search feature that searches package descriptions.
But we can do that too:
.nf
% grep\-available \-F Description foo
.fi
searches for the string "foo" case-sensitively in the descriptions of
all available packages.  If you want case-insensitivity, use
.nf
% grep\-available \-F Description \-i foo
.fi
Truth to be told,
.B apt\-cache
searches package names, too.  We can separately search in the names;
to do so, do
.nf
% grep\-available \-F Package foo
.fi
or
.nf
% grep\-available \-P foo
.fi
which is pretty much the same thing.  We can also search in both
descriptions and names; if match is found in either, the package
record is printed:
.nf
% grep\-available \-P \-F Description foo
.fi
or
.nf
% grep\-available \-F Package \-F Description foo
.fi
This kind of search is the exactly same that
.B apt\-cache
does.
.PP
Here's one thing neither
.B dpkg
nor
.B apt\-cache
do.  Search for a string in the whole
.I status
or
.I available
file (or
any Debian control file, for that matter) and print out all package
records where we have a match.  Try
.nf
% grep\-available dpkg
.fi
sometime and watch how thoroughly
.B dpkg
has infiltrated Debian.
.PP
All the above queries were based on simple substring searches.
But
.B grep\-dctrl
can handle regular expressions in the search pattern.  For example,
to see the status records of all packages with either "apt" or
"dpkg" in their names, use
.nf
% grep\-status \-P \-e 'apt|dpkg'
.fi
.PP
Now that we have seen all these fine and dandy queries, you might
begin to wonder whether it is necessary to always see the whole
paragraph.  You may be, for example, interest only in the dependency
information of the packages involved.  Fine.  To show the depends
lines of all packages maintained by me, do a
.nf
% grep\-available \-F Maintainer \-s Depends 'ajk@debian.org'
.fi
If you want to see the packages' names, too, use
.nf
% grep\-available \-F Maintainer \-s Package,Depends \\
  'ajk@debian.org'
.fi
Note that there must be no spaces in the argument to the
.B \-s
switch.
.PP
More complex queries are also possible.  For example, to see the list of packages
maintained by me and depending on libc6, do
.nf
% grep\-available \-F Maintainer 'ajk@debian.org' \\
   \-a \-F Depends libc6 \-s Package,Depends
.fi
Remember that you can use other UNIX filters to help you, too.  Ever
wondered, who's the most active Debian developer based on the number
of source packages being maintained?  Easy.  You just need to have a
copy of the most recent
.I Sources
file from any Debian mirror.
.nf
% grep\-dctrl \-n \-s Maintainer '' Sources | sort | \\
  uniq \-c | sort \-nr
.fi
This example shows a neat trick: if you want to selectively
show only some field of
.I all
packages, just supply an empty pattern.
.PP
The term "bogopackage" means the count of the packages that a Debian
developer maintains.  To get the bogopackage count for the maintainer
of
.BR dctrl\-tools ,
say
.nf
% grep\-available \-c \-FMaintainer \\
  "`grep\-available \-sMaintainer \-n \-PX dctrl\-tools`"
.fi
.PP
Sometimes it is useful to output the data of several fields on the
same line.  For example, the following command outputs the list of
installed packages, sorted by their
.BR Installed\-Size .
.nf
% grep\-status \-FStatus \-sInstalled\-Size,Package \-n \\
  "install ok installed" -a -FInstalled-Size --gt 0 \\
  | paste \-sd "  \\n" | sort \-n
.fi
Note that there should be exactly 2 spaces in the "  \\n" string.
.PP
Another usual use-case is looking for packages that have another one as
build dependency:
.nf
% grep\-dctrl \-s Package \-F Build\-Depends,Build\-Depends\-Indep \\
  quilt /var/lib/apt/lists/*Sources
.fi
.PP
These examples cover a lot of typical uses of this utility, but not
all possible uses.  Use your imagination!  The building blocks are
there, and if something's missing, let me know.
.SH DIAGNOSTICS
In the absence of errors, the exit code
.B 0
is used if at least one
match was found, and the exit code
.B 1
is used if no matches were found.
If there were errors, the exit code is
.BR 2 ,
with one exception.  If the
.BR \-q ", " \-\-quiet " or " \-\-silent
options are used, the exit code 
.B 0
is used when
a match is found regardless of whether there have been non-fatal
errors.
.PP
These messages are emitted in log levels 
.BR fatal " and " important .
Additional messages may be provided by the system libraries.
.B This list is incomplete.
.IP "\fBA pattern is mandatory"
You must specify a pattern to be searched for.
.IP "\fBmalformed filter"
No filter was specified, but one is required.
.IP "\fBcannot find enough memory"
More memory was needed than was available.  This error may be
transient, that is, if you try again, all may go well.
.IP "\fBcannot suppress field names when showing whole paragraphs"
When you do not use the 
.B \-s
switch,
.B grep\-dctrl
just passes the matching paragraphs through, not touching them any
way.  This means, for example, that you can only use
.B \-n
when you use
.BR \-s .
.IP "\fBinconsistent modifiers of simple filters"
Conflicting modifiers of simple filters were used; for example, perhaps both
.B \-X
and
.B \-e
were specified for the same simple filter.
.IP "\fBmissing ')' in command line"
There were more opening than closing parentheses in the given
filter.
.IP "\fBno such log level"
The argument to
.B \-l
was invalid.
.IP "\fBtoo many file names"
The number of file names specified in the command line exceeded a
compile-time limit.
.IP "\fBtoo many output fields"
The argument to
.B \-s
had too many field names in it.  This number is
limited to 256.
.IP "\fBunexpected ')' in command line"
There was no opening parenthesis that would match some closing
parenthesis in the command line.
.SH FILES
.IP \fI/var/lib/dpkg/available
The default input file of
.BR grep\-available .
.IP \fI/var/lib/dpkg/status
The default input file of
.BR grep\-status .
.SH AUTHOR
The program and this manual page were written by Antti-Juhani
Kaijanaho 
.RI < gaia@iki.fi >.
Bill Allombert
.RI < ballombe@debian.org >
provided one of the examples in the manual page.
.SH "SEE ALSO"
Debian Policy Manual.  Published as the Debian
package
.BR debian\-policy .
Also available in the Debian website.
.PP
.BR apt\-cache (1),
.BR ara (1),
.BR dpkg\-awk (1),
.BR sgrep (1),
.BR dpkg (8)
\" Local variables:
\" mode: nroff
\" End:

