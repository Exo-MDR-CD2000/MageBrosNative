#!/bin/sh
# Mage Bros. Native performance benchmark for the R36S.
#
#   /roms2/ports/MageBrosBench.sh
#
# Runs the generated dense benchmark level for 30 seconds, writes a JSON report beside the game
# and a log next to the normal launcher's, then exits. No input is needed, so it can be started
# from the Ports menu or over SSH.
set -u

base_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
game_dir="$base_dir/magebros"
binary="$game_dir/MageBrosNative"
game_exe="$game_dir/MageBros.exe"
log="$game_dir/benchmark.log"
report="$game_dir/benchmark-report.json"

mkdir -p "$game_dir"
: > "$log"
exec >> "$log" 2>&1
printf -- '--- Mage Bros. Native benchmark ---\n'
printf 'started: %s\n' "$(date 2>/dev/null || echo '?')"

if [ ! -x "$binary" ]; then
    printf 'missing binary: %s\n' "$binary"
    exit 2
fi
if [ ! -f "$game_exe" ]; then
    printf 'MageBros.exe is missing: copy your own copy of the freeware 2.0 build to %s\n' "$game_exe"
    exit 2
fi

export SDL_VIDEO_DRIVER=kmsdrm
export SDL_EGL_LIBRARY=${SDL_EGL_LIBRARY:-/usr/lib/aarch64-linux-gnu/libMali.so}
if [ -f "$game_dir/gamecontrollerdb.txt" ]; then
    export SDL_GAMECONTROLLERCONFIG_FILE="$game_dir/gamecontrollerdb.txt"
fi

cd "$game_dir" || exit 2

# Over SSH the frontend may still hold the console's display. Stop it for the measured run and
# put it back afterwards; when Ports started this script, this is already done or harmless.
restart_es=0
if command -v systemctl >/dev/null 2>&1 && systemctl is-active --quiet emulationstation 2>/dev/null; then
    if sudo -n systemctl stop emulationstation 2>/dev/null; then
        restart_es=1
        sleep 1
    fi
fi

"$binary" --game "$game_exe" --benchmark --benchmark-seconds 30 --benchmark-report "$report"
status=$?
printf 'exit: %s\n' "$status"

if [ "$restart_es" = "1" ]; then
    sudo -n systemctl start emulationstation 2>/dev/null || true
fi
if [ -f "$report" ]; then
    printf 'report: %s\n' "$report"
else
    printf 'report: not written (see %s)\n' "$log"
fi
printf -- '--- done ---\n'
exit "$status"
