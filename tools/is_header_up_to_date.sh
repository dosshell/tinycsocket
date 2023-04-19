#!/bin/sh
# Returns 0 if up to date
PROJPATH="$(dirname "$(readlink -f "$0")")"/..
m4 $PROJPATH/src/tinycsocket.h.m4 | diff $PROJPATH/include/tinycsocket.h -
