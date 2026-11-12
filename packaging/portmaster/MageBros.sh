#!/bin/sh
# Mage Bros. Native for the R36S. Installed as /roms2/ports/MageBros.sh with the game in
# /roms2/ports/magebros/. Put your own MageBros.exe next to MageBrosNative.
#
# The console has no SDL3 and its EGL/GLES entry points live in libMali.so, which SDL does not
# find by itself: SDL_EGL_LIBRARY is the one hint that matters here (SDL_OPENGL_LIBRARY and
# SDL_OPENGL_ES_DRIVER were not needed on this device).
set -u

base_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
game_dir="$base_dir/magebros"
binary="$game_dir/MageBrosNative"
log="$game_dir/magebros.log"

mkdir -p "$game_dir"
: > "$log"
exec >> "$log" 2>&1
printf -- '--- Mage Bros. Native ---\n'
printf 'started: %s\n' "$(date 2>/dev/null || echo '?')"

if [ ! -x "$binary" ]; then
    printf 'missing binary: %s\n' "$binary"
    exit 2
fi

export SDL_VIDEO_DRIVER=kmsdrm
export SDL_EGL_LIBRARY=${SDL_EGL_LIBRARY:-/usr/lib/aarch64-linux-gnu/libMali.so}
if [ -f "$game_dir/gamecontrollerdb.txt" ]; then
    export SDL_GAMECONTROLLERCONFIG_FILE="$game_dir/gamecontrollerdb.txt"
fi

game_exe="$game_dir/MageBros.exe"
if [ ! -f "$game_exe" ]; then
    printf 'MageBros.exe is missing: copy your own copy of the freeware 2.0 build to %s\n' "$game_exe"
    exit 2
fi

cd "$game_dir" || exit 2
"$binary" --game "$game_exe"
status=$?
printf 'exit: %s\n' "$status"
exit "$status"
