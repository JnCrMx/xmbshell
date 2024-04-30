FROM ubuntu:23.10

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    xvfb dbus-x11 mesa-vulkan-drivers libsdl2-2.0-0 libsdl2-mixer-2.0-0 libsdl2-image-2.0-0 \
    libspdlog1.10 libglibmm-2.68-1 libsdbus-c++1 libasan6 llvm \
    vulkan-validationlayers ffmpeg fonts-ubuntu xdotool && \
    rm -rf /var/lib/apt/lists/*
