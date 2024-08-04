FROM ubuntu:24.04

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    clang cmake ccache ninja-build git glslang-tools xxd patch curl gettext ca-certificates libglib2.0-bin \
    libvulkan-dev pkg-config libsystemd-dev libsdl2-dev libsdl2-mixer-dev libsdl2-image-dev libglm-dev \
    libspdlog-dev libfreetype-dev libglibmm-2.68-dev libsdbus-c++-dev libclang-rt-dev && \
    rm -rf /var/lib/apt/lists/*
