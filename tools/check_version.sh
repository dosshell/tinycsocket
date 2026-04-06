#!/bin/sh
# Verifies that TCS_VERSION_TXT matches what generate_version would produce.
# Returns 0 if up to date.
set -e

PROJPATH="$(dirname "$(readlink -f "$0")")"/..

# Get expected version
TAG=$(git -C "$PROJPATH" describe --tags --abbrev=0)
COUNT=$(git -C "$PROJPATH" rev-list --count "$TAG..HEAD")
EXPECTED="${TAG}.${COUNT}"

# Get actual version from header
ACTUAL=$(grep 'TCS_VERSION_TXT' "$PROJPATH/src/tinycsocket_internal.h" | sed 's/.*"\(.*\)".*/\1/')

if [ "$EXPECTED" != "$ACTUAL" ]; then
    echo "Version mismatch: header has \"$ACTUAL\", expected \"$EXPECTED\""
    echo "Run: cmake --build . --target generate_version"
    exit 1
fi

echo "Version OK: $ACTUAL"
