#!/bin/bash

RESOLUTION=1280x720

X +extension RENDER +extension GLX -reset -terminate :99 &
export DISPLAY=:99
export GSETTINGS_SCHEMA_DIR="$PWD/schemas:$GSETTINGS_SCHEMA_DIR"
export XMB_ASSET_DIR=.
export RESOLUTION
export VK_LOADER_DEBUG=debug
