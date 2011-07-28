#!/bin/sh

set -e

${GREP_DCTRL:-grep-dctrl} -sFoo ''
