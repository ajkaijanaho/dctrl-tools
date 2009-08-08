#!/bin/sh

set -e

$GREP_DCTRL -n -X -s Package "Package: gedit"
