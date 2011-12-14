#!/bin/sh

set -e

${GREP_DCTRL:-grep-dctrl} -FFoo a bug652034.in
echo '###'
${GREP_DCTRL:-grep-dctrl} -FFoo b bug652034.in
echo '###'
${GREP_DCTRL:-grep-dctrl} -sFoo -FFoo b bug652034.in
