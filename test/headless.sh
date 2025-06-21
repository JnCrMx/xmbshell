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

key_up() {
    echo "keyboard down 82 1073741906 0\nkeyboard up 82 1073741906 0"
}
key_down() {
    echo "keyboard down 81 1073741905 0\nkeyboard up 81 1073741905 0"
}
key_left() {
    echo "keyboard down 80 1073741904 0\nkeyboard up 80 1073741904 0"
}
key_right() {
    echo "keyboard down 79 1073741903 0\nkeyboard up 79 1073741903 0"
}
key_return() {
    echo "keyboard down 40 13 0\nkeyboard up 40 13 0"
}
key_escape() {
    echo "keyboard down 41 27 0\nkeyboard up 41 27 0"
}

duration=15
( \
    sleep 1.0; key_down; \
    sleep 0.5; key_up; \
    sleep 0.5; key_right; \
    sleep 0.5; key_return; \
    sleep 0.5; key_down; \
    sleep 0.5; key_return; \
    sleep 0.5; key_down; \
    sleep 0.5; key_up; \
    sleep 0.5; key_escape; \
    sleep 0.5; key_escape; \
    sleep 0.5; key_down; \
    sleep 0.5; key_return; \
    sleep 0.5; key_return; \
    sleep 0.5; key_escape; \
    sleep 0.5; key_escape; \
    sleep 0.5; key_left; \
    sleep 0.5; key_down; \
    sleep 0.5; key_return; \
    sleep 0.5; key_right; \
    sleep 0.5; key_return; \
) | dbus-launch timeout $duration ./build/xmbshell | tee build/test-log.txt

frames=$(ls -1q $DREAMRENDER_HEADLESS_OUTPUT_DIR | wc -l)
framerate=$(($frames / $duration))

echo "Duration: $duration"
echo "Frames: $frames"
echo "Framerate: $framerate"

ffmpeg -threads 1 -loglevel verbose -framerate $framerate \
    -i "$DREAMRENDER_HEADLESS_OUTPUT_DIR"/"%$format_spec.bmp" \
    -c:v vp9 -cpu-used 5 build/test-output.webm \
    -loop 65535 -compression_level 2 -quality 75 build/test-output.webp
