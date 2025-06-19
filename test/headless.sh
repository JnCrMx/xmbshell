#!/bin/bash

format_spec="05d"

export DREAMRENDER_HEADLESS=1
export DREAMRENDER_HEADLESS_WIDTH=800
export DREAMRENDER_HEADLESS_HEIGHT=600
export DREAMRENDER_HEADLESS_OUTPUT_DIR="output/"
export DREAMRENDER_HEADLESS_OUTPUT_PATTERN="{:$format_spec}.bmp"

export GSETTINGS_SCHEMA_DIR="$PWD/schemas:$GSETTINGS_SCHEMA_DIR"
export XMB_ASSET_DIR=.
export SPDLOG_LEVEL=debug

duration=15
dbus-launch timeout $duration ./build/xmbshell < /dev/null | tee build/test-log.txt

frames=$(ls -1q $DREAMRENDER_HEADLESS_OUTPUT_DIR | wc -l)
framerate=$(($frames / $duration))

echo "Duration: $duration"
echo "Frames: $frames"
echo "Framerate: $framerate"

ffmpeg -framerate $framerate \
    -i "$DREAMRENDER_HEADLESS_OUTPUT_DIR"/"%$format_spec.bmp" \
    build/test-output.mp4 \
    build/test-output.webm \
    -loop 65535 build/test-output.webp

rm -r "$DREAMRENDER_HEADLESS_OUTPUT_DIR"
