#!/bin/sh
#     dctrl-tools - Debian control file inspection tools
#     Copyright (C) 2004 Antti-Juhani Kaijanaho
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

aptcache=/usr/bin/apt-cache

if [ $# -ge 1 ]
then
    if [ "x$@" = "x--version" ]
    then
	echo "update-avail (dctrl-tools) VERSION"
    else
	echo "Usage:"
	echo "  $0"
	echo "  $0 --version"
	echo "  $0 --help"
    fi
    exit 0
fi

if [ `id -u` -ne "0" ]
then
    echo "$0: root privileges are required"
    exit 1
fi

if [ ! -x $aptcache ]
then
    echo "$0: $aptcache not executable, aborting" 1>&2
    exit 1
fi

tf=`mktemp -t apt-available.XXXXXX` || exit 1
echo -n "Merging available database in $tf..."
$aptcache dumpavail >> $tf
echo "done."
dpkg --update-avail $tf
rm $tf
