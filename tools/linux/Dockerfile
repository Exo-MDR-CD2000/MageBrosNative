# The Linux desktop build environment: Debian 13 has the glibc the Linux package targets
# (2.41) and every SDL3 dependency the desktop backends need. SDL3 itself is built from the
# pinned source and linked statically; this image only provides headers and build tools.
ARG BASE_IMAGE=debian:13-slim
FROM ${BASE_IMAGE}

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build git ca-certificates pkg-config \
        libasound2-dev libpulse-dev libpipewire-0.3-dev \
        libx11-dev libxext-dev libxrandr-dev libxi-dev libxcursor-dev libxfixes-dev \
        libxss-dev libxtst-dev libxkbcommon-dev libxkbcommon-x11-dev \
        libwayland-dev wayland-protocols libdecor-0-dev \
        libgl1-mesa-dev libegl1-mesa-dev libgles2-mesa-dev \
        libudev-dev libdbus-1-dev libibus-1.0-dev libusb-1.0-0-dev \
        libfribidi-dev libthai-dev \
    && rm -rf /var/lib/apt/lists/*
