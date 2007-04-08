#!/bin/sh
#     dctrl-tools - Debian control file inspection tools
#     Copyright (C) 2007 Antti-Juhani Kaijanaho
#
#     This program is free software; you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation; either version 2 of the License, or
#     (at your option) any later version.
#
#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#
#     You should have received a copy of the GNU General Public License
#     along with this program; if not, write to the Free Software
#     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

set -e

trap "rm .testout .diffout 2>/dev/null" \
    EXIT ABRT BUS FPE HUP ILL QUIT SEGV TERM

GREP_DCTRL=`pwd`/grep-dctrl/grep-dctrl
export GREP_DCTRL

cd tests

if [ $# -ge 1 ] ; then
    tests=$@
else
    tests=`ls -1 *.sh | sort`
fi

rv=0

for tst in $tests ; do
    tst_base=`basename $tst .sh`
    tst_in=$tst_base.in
    tst_out=$tst_base.out
    tst_err=$tst_base.fails
    if [ ! -r $tst_in ] ; then
        tst_in=/dev/null
    fi
    if [ ! -r $tst_out ] && [ ! -r $tst_err ] ; then
        echo 1>&2 "neither $tst_out nor $tst_err exists"
        exit 1
    fi
    if [ -r $tst_err ]; then
        echo -n "$0: Test case $tst_base (expecting failure)..."
    else
        echo -n "$0: Test case $tst_base (expecting success)..."
    fi
    if sh $tst < $tst_in > .testout ; then
        if diff -au $tst_out .testout > .diffout ; then
            echo "ok."
        else
            echo "FAILED."
            cat .diffout
            rv=1
        fi
    else
        if [ -r $tst_err ] ; then
            echo "ok."
        else
            echo "FAILED."
            rv=1
        fi
    fi
done

exit $rv
