#!/bin/sh

set -e

$GREP_DCTRL -FMaintainer alfie -sPackage,Installed-Size,Description -d 0001.in|
    $SORT_DCTRL -kInstalled-Size:v -