#!/bin/sh

set -e

$JOIN_DCTRL -j Package -o 0,1.Version:Old-Version,2.Version:New-Version 0007.stat 0007.pkg 