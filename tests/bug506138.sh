#!/bin/sh

set -e

LC_ALL=C
export LC_ALL

$GREP_DCTRL -FFoo bar baz -o -FBar xyzzy qwerty 2>&1 | 
grep "file names are not" >/dev/null
