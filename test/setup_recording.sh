#!/bin/bash

RESOLUTION=720x405

Xvfb :99 -screen 0 ${RESOLUTION}x24 -reset -terminate &
export DISPLAY=:99
export GSETTINGS_SCHEMA_DIR="$PWD/schemas:$GSETTINGS_SCHEMA_DIR"
export XMB_ASSET_DIR=.
export RESOLUTION
