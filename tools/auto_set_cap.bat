#!/bin/bash

# sudo apt install inotify-tools
# Disable apparmor (/etc/default/grub, add `apparmor=0` to GRUB_CMDLINE_LINUX, then update-grub)
# Add `set startup-with-shell off` to ~/.gdbinit to prevent gdb from inheriting capabilities from bash/zsh
# sudo setcap cap_net_admin,cap_net_raw=eip /usr/bin/gdb
# sudo ./tools/auto_set_cap.sh ./build/tests/tests

FILE_TO_WATCH="$1"

setcap cap_net_admin,cap_net_raw=eip "$FILE_TO_WATCH"
echo "Permissions updated for $FILE_TO_WATCH"

while true; do
    inotifywait -e modify "$FILE_TO_WATCH"
    setcap cap_net_admin,cap_net_raw=eip "$FILE_TO_WATCH"
    echo "Permissions updated for $FILE_TO_WATCH"
done
