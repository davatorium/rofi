#!/usr/bin/env bash

export ROFI_PNG_OUTPUT=out.png
rofi -show run &
RPID=$!

# send enter.
sleep 5
xdotool key 't'
sleep 0.4
xdotool key 'r'
sleep 0.4
xdotool key 'u'
sleep 0.4
xdotool key Alt+Shift+s
sleep 0.4
xdotool key Return

# Get result, kill xvfb
wait "${RPID}"
RETV=$?

if [ ! -f out.png ]
then
    echo "Failed to create screenshot"
    exit 1
fi
exit "${RETV}"
