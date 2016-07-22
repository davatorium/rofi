#!/usr/bin/env bash

echo -e -n "aap\nnoot\nmies" | rofi -width -30 -dmenu -multi-select > output.txt & 
RPID=$!

# send enter.
sleep 5;
xdotool key 'c'
sleep 0.2
xdotool key 'o'
sleep 0.2
xdotool key 'f'
sleep 0.2
xdotool key 'f'
sleep 0.2
xdotool key 'e'
sleep 0.2
xdotool key 'e'
sleep 0.2
xdotool key Return


#  Get result, kill xvfb
wait ${RPID}
RETV=$?
OUTPUT=$(cat output.txt | tr '\n' ' ')
if [ "${OUTPUT}" != 'coffee ' ]
then
    exit 1
fi

OUTPUT=$(rofi -dump-xresources)
if [ -z "${OUTPUT}" ]
then
    exit 1
fi

exit ${RETV}
