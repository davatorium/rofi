#!/usr/bin/env bash

# wait till it is up, run rofi with error message
rm -f output.txt
sleep 1;
echo -e -n "aap\nnoot\nmies" | rofi -dmenu -no-custom -kb-custom-1 F5 -kb-move-front "" -kb-custom-2 "Control+a" > output.txt &
RPID=$!

# send enter.
sleep 5;
xdotool key 'q' 
sleep 0.4
xdotool key Return
sleep 0.4
xdotool key F5
sleep 0.4
xdotool key "Control+a"
sleep 0.4
xdotool key Escape

# Get result, kill xvfb
wait "${RPID}"
RETV=$?
OUTPUT=$(tr '\n' ' ' < output.txt)
if [ "${OUTPUT}" != '' ]
then
    echo "Got: '${OUTPUT}' expected nothing"
    exit 1
fi
if [ "${RETV}" != 1 ]
then
    exit 1 
fi
