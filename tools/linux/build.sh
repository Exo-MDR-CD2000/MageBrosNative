#!/bin/sh
# Builds MageBrosNative for Linux desktop on any machine with Docker.
#
# The package targets Debian 13 (glibc 2.41) and newer: the build runs in a Debian 13 x86_64
# container so the artifact matches the documented baseline. SDL3 3.4.18 is built from the
# pinned source and linked statically; X11 and Wayland, ALSA/Pulse/PipeWire, EGL and the C++
# runtime remain system libraries.
#
#   tools/linux/build.sh              build, test and package
#   MBN_LINUX_SKIP_TESTS=1 ...        build and package only
#   MBN_LINUX_BASE=ubuntu:22.04 ...   use another base image (a contributor experiment)
#
# The result is build-linux/MageBrosNative and
# build-linux/MageBrosNative-<version>-linux-x86_64.tar.gz.
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
base_image=${MBN_LINUX_BASE:-debian:13-slim}
image=${MBN_LINUX_IMAGE:-mbn-linux:debian13}
dockerfile="$project_dir/tools/linux/Dockerfile"

if ! docker image inspect "$image" >/dev/null 2>&1; then
    echo "Building the Linux build image ($image from $base_image) ..."
    docker build --platform linux/amd64 --build-arg BASE_IMAGE="$base_image" \
        -t "$image" -f "$dockerfile" "$project_dir/tools/linux" >/dev/null
fi

run_tests=1
[ "${MBN_LINUX_SKIP_TESTS:-0}" = "1" ] && run_tests=0

docker run --rm --platform linux/amd64 \
    --user "$(id -u):$(id -g)" \
    -e HOME=/tmp \
    -e MBN_LINUX_RUN_TESTS="$run_tests" \
    -v "$project_dir:/work" \
    -w /work \
    "$image" sh -eu -c '
        cmake -S . -B build-linux -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DMBN_SDL_FROM_SOURCE=ON \
            -DMBN_BUILD_GAME=ON \
            -DMBN_GLES=OFF \
            -DMBN_BUILD_R36S_PROBE=OFF \
            -DMBN_TEXTURE_DIAG=OFF \
            -DSDL_X11=ON -DSDL_WAYLAND=ON -DSDL_KMSDRM=OFF \
            -DSDL_ALSA=ON -DSDL_PULSEAUDIO=ON -DSDL_PIPEWIRE=ON \
            -DSDL_JACK=OFF -DSDL_SNDIO=OFF \
            -DSDL_VULKAN=OFF \
            -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF -DSDL_TEST_LIBRARY=OFF
        cmake --build build-linux
        if [ "$MBN_LINUX_RUN_TESTS" = "1" ]; then
            ctest --test-dir build-linux --output-on-failure
        fi
        cd build-linux && cpack
    '

echo
echo "Built: $project_dir/build-linux/MageBrosNative"
file "$project_dir/build-linux/MageBrosNative"
