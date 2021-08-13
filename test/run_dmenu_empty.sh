#!/usr/bin/env bash

rofi -dmenu & < /dev/null
RPID=$!
sleep 4
xdotool key Return
# Get result, kill xvfb
wait "${RPID}"
RETV=$?

if [ "${RETV}" -eq 0 ]
then
    exit 0
else
    exit 1
fi
