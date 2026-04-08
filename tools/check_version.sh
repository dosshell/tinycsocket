#!/bin/sh
# Verifies TCS_VERSION_TXT matches git. Format: <tag>.<first-parent count>
set -e

PROJPATH="$(dirname "$(readlink -f "$0")")"/..
TAG=$(git -C "$PROJPATH" describe --tags --abbrev=0)
ACTUAL=$(grep 'const TCS_VERSION_TXT =' "$PROJPATH/src/tinycsocket_internal.h" | sed 's/.*"\(.*\)".*/\1/')

if [ "${GITHUB_REF:-}" = "refs/heads/master" ]; then
    COUNT=$(git -C "$PROJPATH" rev-list --count --first-parent "$TAG..HEAD")
else
    COUNT=$(( $(git -C "$PROJPATH" rev-list --count --first-parent "$TAG..origin/master") + 1 ))
fi

EXPECTED="${TAG}.${COUNT}"

if [ "$ACTUAL" != "$EXPECTED" ]; then
    echo "Version mismatch: header has \"$ACTUAL\", expected \"$EXPECTED\""
    exit 1
fi

echo "Version OK: $ACTUAL"
