#!/bin/bash

if [ -z "$1" ]; then
  echo "Usage: $0 <snap-name>"
  exit 1
fi

SNAPNAME=$1

url="$(curl -s -H 'X-Ubuntu-Series: 16' -H "X-Ubuntu-Architecture: $ARCH" "https://api.snapcraft.io/api/v1/snaps/details/$SNAPNAME" | jq '.download_url' -r)"
curl -s -H 'X-Ubuntu-Series: 16' -H "X-Ubuntu-Architecture: $ARCH" "$url" -o "$SNAPNAME.snap"
if [ $? -ne 0 ]; then
  echo "Failed to download $SNAPNAME"
  exit 1
fi

mkdir -p /snap/"$SNAPNAME"
unsquashfs -d /snap/$SNAPNAME/current $SNAPNAME.snap
if [ $? -ne 0 ]; then
  echo "Failed to unsquash $SNAPNAME"
  exit 1
fi

rm $SNAPNAME.snap
