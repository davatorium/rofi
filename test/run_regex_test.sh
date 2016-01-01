#!/usr/bin/env bash

echo -en "nooty\naap\nnoot\nmies" | rofi -regex -dmenu   > output.txt  &
RPID=$!

# send enter.
sleep 5;
xdotool key Shift+'6' 
sleep 0.4
xdotool key 'n'
sleep 0.4
xdotool key 'o'
sleep 0.4
xdotool key 'o'
sleep 0.4
xdotool key 't'
sleep 0.4
xdotool key Shift+'4'
sleep 0.4
xdotool key Return

#  Get result, kill xvfb
wait ${RPID}
RETV=$?
OUTPUT=$(cat output.txt)
if [ "${OUTPUT}" != 'noot' ]
then
    echo "Got: '${OUTPUT}' expected 'noot'"
    exit 1
fi

exit ${RETV}
