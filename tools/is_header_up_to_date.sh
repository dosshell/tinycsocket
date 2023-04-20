#!/bin/sh
# Returns 0 if up to date
PROJPATH="$(dirname "$(readlink -f "$0")")"/..
cd $PROJPATH/src
m4 tinycsocket.h.m4 | diff $PROJPATH/include/tinycsocket.h -
