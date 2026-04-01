#!/bin/bash

set -e

echo "Elevating permissions for cap_net_raw and cap_net_admin capabilities (AVTP)."
echo "dms-gui is still running as normal user, only these capabilities are elevated."

home_saved="$HOME"
user_saved="$USER"

wayland_saved="$WAYLAND_DISPLAY"
xdg_saved="$XDG_RUNTIME_DIR"

display_saved="$DISPLAY"
xauth_saved="$XAUTHORITY"

pkexec capsh \
    --keep=1 \
    --user="$user_saved" \
    --inh=cap_net_raw,cap_net_admin \
    --addamb=cap_net_raw,cap_net_admin \
    -- -c "env \
        HOME='$home_saved' \
        USER='$user_saved' \
        WAYLAND_DISPLAY='$wayland_saved' \
        XDG_RUNTIME_DIR='$xdg_saved' \
        DISPLAY='$display_saved' \
        XAUTHORITY='$xauth_saved' \
        /usr/bin/gdb $@"
