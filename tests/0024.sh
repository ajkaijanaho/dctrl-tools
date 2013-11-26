#!/bin/sh

set -e

LC_ALL=C
export LC_ALL

$GREP_DCTRL -FPackage aaab -L 0014.in 0015.in
