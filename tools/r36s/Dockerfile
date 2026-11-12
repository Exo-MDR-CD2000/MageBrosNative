# The R36S build environment: Debian 13 has the console's glibc (2.41) and a native aarch64
# toolchain. The game is built here with a static SDL3 and linked against only the libraries
# the console already has.
FROM debian:13-slim

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build git ca-certificates pkg-config \
        libdrm-dev libgbm-dev libasound2-dev libudev-dev \
    && rm -rf /var/lib/apt/lists/*
