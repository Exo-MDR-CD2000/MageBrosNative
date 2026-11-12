#!/bin/sh
# Copies the built game and its launcher to the R36S over SSH.
#
#   tools/r36s/build.sh
#   tools/r36s/install.sh                     (MBN_R36S_HOST defaults to ark@192.168.0.104)
#   tools/r36s/install.sh ark@other-host
#
# It writes /roms2/ports/MageBros.sh and /roms2/ports/magebros/ on the console's second card,
# where the Ports menu reads from. Nothing on the system is replaced. Your own MageBros.exe is
# not uploaded: copy it into /roms2/ports/magebros/ yourself.
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
host=${1:-${MBN_R36S_HOST:-ark@192.168.0.104}}
remote_ports=${MBN_R36S_PORTS:-/roms2/ports}
binary="$project_dir/build-r36s/MageBrosNative"

if [ ! -f "$binary" ]; then
    echo "Missing $binary: run tools/r36s/build.sh first." >&2
    exit 2
fi

ssh "$host" "mkdir -p '$remote_ports/magebros'"
# Upload beside the target and rename: the old binary may be running (ETXTBSY on overwrite),
# and the launcher may be mid-session.
scp -q "$binary" "$host:$remote_ports/magebros/MageBrosNative.new"
scp -q "$project_dir/packaging/portmaster/gamecontrollerdb.txt" "$host:$remote_ports/magebros/"
scp -q "$project_dir/packaging/portmaster/MageBros.sh" "$host:$remote_ports/MageBros.sh.new"
ssh "$host" "mv -f '$remote_ports/magebros/MageBrosNative.new' '$remote_ports/magebros/MageBrosNative' && mv -f '$remote_ports/MageBros.sh.new' '$remote_ports/MageBros.sh'"
ssh "$host" "chmod +x '$remote_ports/MageBros.sh' '$remote_ports/magebros/MageBrosNative'"
echo "Installed to $host:$remote_ports (put your MageBros.exe in $remote_ports/magebros/)."
