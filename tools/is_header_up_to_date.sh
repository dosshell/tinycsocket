#!/bin/sh
# Returns 0 if the committed header matches what would be generated
PROJPATH="$(dirname "$(readlink -f "$0")")"/..
TMPFILE=$(mktemp)
trap "rm -f $TMPFILE" EXIT

# Generate to temp file
cmake -DSRC_DIR=$PROJPATH -DOUTPUT=$TMPFILE -P $PROJPATH/cmake/GenerateHeader.cmake
diff $PROJPATH/include/tinycsocket.h $TMPFILE
