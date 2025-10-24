# xmbshell [![status-badge](https://woodpecker.jcm.re/api/badges/16/status.svg)](https://woodpecker.jcm.re/repos/16) [![xmbshell](https://snapcraft.io/xmbshell/badge.svg)](https://snapcraft.io/xmbshell)

A desktop shell mimicking the look and functionality of the XrossMediaBar dedicated to my cute girlfriend Laura!

[![Get it from the Snap Store](https://snapcraft.io/en/dark/install.svg)](https://snapcraft.io/xmbshell)

## Direct downloads of latest automatic build

**Linux**
||amd64|arm64|
|---|---|---|
|AppImage|[XMB_Shell-x86_64.AppImage](https://woodpecker.web.garage.jcm.re/artifacts/XMB-OS/xmbshell/main/public/XMB_Shell-x86_64.AppImage)|[XMB_Shell-aarch64.AppImage](https://woodpecker.web.garage.jcm.re/artifacts/XMB-OS/xmbshell/main/public/XMB_Shell-aarch64.AppImage)|
|Snap package|[xmbshell_beta_amd64.snap](https://woodpecker.web.garage.jcm.re/artifacts/XMB-OS/xmbshell/main/public/xmbshell_beta_amd64.snap)|[xmbshell_beta_arm64.snap](https://woodpecker.web.garage.jcm.re/artifacts/XMB-OS/xmbshell/main/public/xmbshell_beta_arm64.snap)|
|Ubuntu 24.04 (.deb)|[xmbshell-beta-noble-amd64.deb](https://woodpecker.web.garage.jcm.re/artifacts/XMB-OS/xmbshell/main/public/xmbshell-beta-noble-amd64.deb)|[xmbshell-beta-noble-arm64.deb](https://woodpecker.web.garage.jcm.re/artifacts/XMB-OS/xmbshell/main/public/xmbshell-beta-noble-arm64.deb)|

**Windows**
||amd64|
|---|---|
|portable ZIP|[xmbshell-beta-windows.zip](https://woodpecker.web.garage.jcm.re/artifacts/XMB-OS/xmbshell/main/public/xmbshell-beta-windows.zip)|

![Preview](https://woodpecker.web.garage.jcm.re/artifacts/XMB-OS/xmbshell/main/public/test-output.webp)

## Features
- Awesome XMB background wave
- Basic file management
- Launch other apps
- Basic image viewer
- WIP video player

**Some of the icons made by Laura herself!**

## Building from source

### Ubuntu 24.04

First, install the following build-time dependencies:
```
sudo apt install clang clang-tools cmake ninja-build fonts-ubuntu glslang-tools patch curl wget gettext libglib2.0-bin libvulkan-dev pkg-config libsystemd-dev libsdl2-dev libsdl2-mixer-dev libsdl2-image-dev libglm-dev libspdlog-dev libfreetype-dev libglibmm-2.68-dev libsdbus-c++-dev libboost-dev libavformat-dev libavcodec-dev libavutil-dev libavfilter-dev libswscale-dev libswresample-dev libpostproc-dev libavdevice-dev vulkan-validationlayers
```

Then clone, configure and build the project:
```
git clone https://github.com/JnCrMx/xmbshell.git
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -B build
cmake --build build
glib-compile-schemas schemas/
```
(This project uses C++20 named modules, so I recommend compiling with a modern Clang and Ninja) <br>
Using `CMAKE_BUILD_TYPE=Debug` instructs CMake to compile the app in debug mode, which will cause it to automatically load
the Vulkan Validation Layers.

Finally, run the app directly from the build directory using the following command
```
GSETTINGS_SCHEMA_DIR="$PWD/schemas:$GSETTINGS_SCHEMA_DIR" XMB_ASSET_DIR=. ./build/xmbshell
```

## Acknowledgements
- [OpenXMB](https://github.com/phenom64/OpenXMB), a very cool fork on XMBShell, from which I ported features back to this repository and copied quite a bit of code.
- [RetroArch](https://github.com/libretro/RetroArch), from which I took the XMB wave shader.
- [i18n++](https://github.com/zauguin/i18n-cpp) for internationalization (forked with some fixes [here](https://github.com/JnCrMx/i18n-cpp))
- [sdbus-c++](https://github.com/Kistler-Group/sdbus-cpp) for D-Bus integration
- [argparse](https://github.com/p-ranav/argparse) for argument parsing
- [AvCpp](https://github.com/h4tr3d/avcpp) and [FFmpeg](https://www.ffmpeg.org/) for video decoding
- [glibmm](https://gitlab.gnome.org/GNOME/glibmm) for GSetting and desktop integration
- [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp) for C++ Vulkan bindings
- [spdlog](https://github.com/gabime/spdlog) for easy and pretty logging
- [SDL2](https://libsdl.org/) for (controller) input and window system integration
- [FreeType](https://freetype.org/) for font rendering
- [glm](https://glm.g-truc.net/) for vector and matrix math operations
- [VulkanMemoryAllocator-Hpp](https://github.com/YaaZ/VulkanMemoryAllocator-Hpp) and [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) for easy and efficient Vulkan memory allocations

## License
XMBShell, a console-like desktop shell
Copyright (C) 2025 - JCM

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
