#!/bin/sh
# Builds MageBrosNative for the R36S on any machine with Docker.
#
# The console is aarch64 Debian 13 with glibc 2.41 and no SDL3, so the build runs in a Debian 13
# arm64 container (native on Apple Silicon) with SDL3 3.4.18 built from the pinned source and
# linked statically. Only KMSDRM video and ALSA audio are compiled in: there is no X11 or
# Wayland on the device.
#
#   tools/r36s/build.sh              build and run the tests
#   MBN_R36S_SKIP_TESTS=1 ...        build only
#
# The result is build-r36s/MageBrosNative, ready to copy to the console.
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
image=${MBN_R36S_IMAGE:-mbn-r36s:debian13}
dockerfile="$project_dir/tools/r36s/Dockerfile"

if ! docker image inspect "$image" >/dev/null 2>&1; then
    echo "Building the R36S build image ($image) ..."
    docker build --platform linux/arm64 -t "$image" -f "$dockerfile" "$project_dir/tools/r36s" >/dev/null
fi

run_tests=1
[ "${MBN_R36S_SKIP_TESTS:-0}" = "1" ] && run_tests=0

docker run --rm --platform linux/arm64 \
    --user "$(id -u):$(id -g)" \
    -e HOME=/tmp \
    -e MBN_R36S_RUN_TESTS="$run_tests" \
    -v "$project_dir:/work" \
    -w /work \
    "$image" sh -eu -c '
        cmake -S . -B build-r36s -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DMBN_SDL_FROM_SOURCE=ON \
            -DMBN_BUILD_GAME=ON \
            -DMBN_BUILD_R36S_PROBE=ON \
            -DMBN_GLES=ON \
            -DSDL_X11=OFF -DSDL_WAYLAND=OFF -DSDL_KMSDRM=ON \
            -DSDL_UNIX_CONSOLE_BUILD=ON \
            -DSDL_ALSA=ON -DSDL_PULSEAUDIO=OFF -DSDL_PIPEWIRE=OFF \
            -DSDL_JACK=OFF -DSDL_SNDIO=OFF \
            -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF -DSDL_TEST_LIBRARY=OFF
        cmake --build build-r36s
        if [ "$MBN_R36S_RUN_TESTS" = "1" ]; then
            ctest --test-dir build-r36s --output-on-failure
        fi
    '

echo
echo "Built: $project_dir/build-r36s/MageBrosNative"
file "$project_dir/build-r36s/MageBrosNative"
