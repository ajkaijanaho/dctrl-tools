#!/bin/sh

set -e

${GREP_DCTRL:-grep-dctrl} -XP glotski -I -sDescription 0001.in 0001.in
