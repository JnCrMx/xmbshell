# xmbshell

A desktop shell mimicking the look and functionality of the XrossMediaBar dedicated to my cute girlfriend Laura!

[![Get it from the Snap Store](https://snapcraft.io/en/dark/install.svg)](https://snapcraft.io/xmbshell)

**Direct downloads of latest automatic build:**
||amd64|arm64|
|---|---|---|
|AppImage|[XMB_Shell-x86_64.AppImage](https://woodpecker.web.garage.jcm.re/artifacts/XMB-OS/xmbshell/main/public/XMB_Shell-x86_64.AppImage)|[XMB_Shell-aarch64.AppImage](https://woodpecker.web.garage.jcm.re/artifacts/XMB-OS/xmbshell/main/public/XMB_Shell-aarch64.AppImage)|
|Snap package|[xmbshell_beta_amd64.snap](https://woodpecker.web.garage.jcm.re/artifacts/XMB-OS/xmbshell/main/public/xmbshell_beta_amd64.snap)|[xmbshell_beta_arm64.snap](https://woodpecker.web.garage.jcm.re/artifacts/XMB-OS/xmbshell/main/public/xmbshell_beta_arm64.snap)|
|Ubuntu 24.04 (.deb)|[xmbshell-beta-noble-amd64.deb](https://woodpecker.web.garage.jcm.re/artifacts/XMB-OS/xmbshell/main/public/xmbshell-beta-noble-amd64.deb)|[xmbshell-beta-noble-arm64.deb](https://woodpecker.web.garage.jcm.re/artifacts/XMB-OS/xmbshell/main/public/xmbshell-beta-noble-arm64.deb)|

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
