#!/bin/sh

set -e

LC_ALL=C
export LC_ALL

$GREP_DCTRL --ignore-parse-errors '' 0010.in
