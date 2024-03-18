#!/bin/bash

set -e

# Install dependencies
pacman -Sy
pacman -S --noconfirm --needed git wget cmake ninja glslang \
    vulkan-headers vulkan-icd-loader sdl2 sdl2_mixer sdl2_image freetype2 \
    glm spdlog glibmm-2.68 gtk3 ttf-ubuntu-font-family

cd /src
cmake -B build-appimage -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr

cd build-appimage
ninja

# Create AppImage
mkdir AppDir || true
DESTDIR="$PWD/AppDir" ninja install
cp /usr/share/fonts/ubuntu/Ubuntu-R.ttf AppDir/usr/share/xmbshell/Ubuntu-R.ttf

wget -c https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20240109-1/linuxdeploy-x86_64.AppImage
wget -c https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh
chmod +x linuxdeploy-x86_64.AppImage linuxdeploy-plugin-gtk.sh

DEPLOY_GTK_VERSION=3 ./linuxdeploy-x86_64.AppImage --appdir AppDir --plugin gtk --output appimage
