#!/bin/sh

set -e

${GREP_DCTRL:-grep-dctrl} -FPackage \( foo -a ! bar \)
