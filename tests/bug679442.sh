#!/bin/sh

set -e

LC_ALL=C
export LC_ALL

$JOIN_DCTRL -j Mergefield bug679442.in bug679442.in
