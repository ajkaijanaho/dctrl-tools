#!/bin/sh

set -e

$GREP_DCTRL --ensure-dctrl -sDescription '' < 0001.in
