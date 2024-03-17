#!/bin/bash

set -e

podman run -it --rm -e APPIMAGE_EXTRACT_AND_RUN=1 -v $(pwd):/src docker.io/ianburgwin/steamos:base-devel \
    /src/scripts/appimage/in_container.sh
