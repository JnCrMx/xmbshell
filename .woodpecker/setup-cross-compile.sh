#!/bin/bash

if [ "$TARGETARCH" == "$BUILDARCH" ]; then
    echo "TARGETARCH is the same as BUILDARCH, skipping cross-compilation setup"
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends dpkg-dev
    touch /toolchain.cmake
    exit 0
fi

dpkg --add-architecture $TARGETARCH

sed -i "/Types: deb/a Architectures: $BUILDARCH" /etc/apt/sources.list.d/ubuntu.sources
SUITES=$(cat /etc/apt/sources.list.d/ubuntu.sources | grep -E '^[^#]' | grep -Eo 'Suites: .*' | cut -d' ' -f2- | tr '\n' ' ')

URL=""
if [ "$TARGETARCH" == "amd64" ] || [ "$TARGETARCH" == "i386" ]; then
    URL="http://archive.ubuntu.com/ubuntu/"
else
    URL="http://ports.ubuntu.com/"
fi

cat > /etc/apt/sources.list.d/ubuntu-$TARGETARCH.sources <<EOF
Types: deb
Architectures: $TARGETARCH
URIs: $URL
Suites: $SUITES
Components: main universe restricted multiverse
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
EOF

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
