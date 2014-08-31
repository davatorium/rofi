#!/usr/bin/env bash

xrdb -load ../doc/example.xresources 
echo -e -n "aap\nnoot\nmies" | rofi -width -30 -dmenu > output.txt & 
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
xdotool key Shift+Return
xdotool key Return

#  Get result, kill xvfb
wait ${RPID}
RETV=$?
OUTPUT=$(cat output.txt | tr '\n' ' ')
if [ "${OUTPUT}" != 'coffee coffee ' ]
then
    exit 1
fi
exit ${RETV}
