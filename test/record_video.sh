#!/bin/bash

source test/setup_recording.sh

dbus-launch timeout 60 ./build/xmbshell | tee build/test-log.txt &
sleep 15
ffmpeg -threads 1 -video_size $RESOLUTION -framerate 30 -f x11grab -i :99 -preset ultrafast -qp 0 -t 45 -y build/test-output.mkv &
xdotool key --delay 500 Up Down Right Return Down Down Up Escape Down Return Down Up Escape Right Right Right Right Right Down Down Down Up Up Up
wait

ffmpeg -threads 1 -i build/test-output.mkv build/test-output.mp4
