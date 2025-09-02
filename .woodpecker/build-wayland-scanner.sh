#!/bin/bash

URL="${WAYLANDPP_URL:-https://github.com/NilsBrause/waylandpp.git}"
VERSION="${WAYLANDPP_VERSION:-master}"

cd /tmp

git clone --depth 1 "$URL" waylandpp
cd waylandpp
git checkout "$VERSION"

cmake -B build -G Ninja -DBUILD_SCANNER=ON -DBUILD_LIBRARIES=OFF -DBUILD_DOCUMENTATION=OFF
cmake --build build

mkdir -p /usr/local/bin
cp build/wayland-scanner++ /usr/local/bin/wayland-scanner++

cd /tmp
rm -rf waylandpp
