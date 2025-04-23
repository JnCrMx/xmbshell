#!/bin/bash

if [ "$TARGETARCH" == "$BUILDARCH" ]; then
    echo "TARGETARCH is the same as BUILDARCH, skipping cross-compilation setup"
    touch /toolchain.cmake
    exit 0
fi

dpkg --add-architecture $TARGETARCH

if [ -f /etc/apt/sources.list.d/ubuntu.sources ]; then
    sed -i "/Types: deb/a Architectures: $BUILDARCH" /etc/apt/sources.list.d/ubuntu.sources
fi

VERSION_CODENAME=$(cat /etc/os-release | grep VERSION_CODENAME | cut -d'=' -f2)
if [ "$TARGETARCH" == "amd64" ]; then
    echo "deb [arch=$TARGETARCH] http://archive.ubuntu.com/ubuntu/ $VERSION_CODENAME main universe multiverse" >> /etc/apt/sources.list.d/forein.list
else
    echo "deb [arch=$TARGETARCH] http://ports.ubuntu.com/ $VERSION_CODENAME main universe multiverse" >> /etc/apt/sources.list.d/forein.list
fi
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends dpkg-dev

TARGET_TRIPLE=$(dpkg-architecture -a$TARGETARCH -qDEB_HOST_GNU_TYPE)

tee /toolchain.cmake <<-EOF
MESSAGE(STATUS "Cross-compiling for $TARGETARCH")

SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR $TARGETARCH)

SET(CMAKE_C_COMPILER_TARGET $TARGET_TRIPLE)
SET(CMAKE_CXX_COMPILER_TARGET $TARGET_TRIPLE)

SET(CMAKE_LIBRARY_ARCHITECTURE $TARGETARCH)
SET(ENV{PKG_CONFIG_PATH} /usr/lib/$TARGET_TRIPLE/pkgconfig/:\$ENV{PKG_CONFIG_PATH})
EOF
