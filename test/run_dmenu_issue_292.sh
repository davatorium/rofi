#!/usr/bin/env bash

# wait till it is up, run rofi with error message
sleep 1;
echo {0..100} | tr " " "\n" | rofi -dmenu  > output.txt &
RPID=$!

# send enter.
sleep 5;
xdotool key '2' 
sleep 0.4
xdotool key Shift+Return
#2
xdotool key Shift+Return
#20
xdotool key Shift+Return
#21
xdotool key Shift+Return
#22
xdotool key Shift+Return
#23
xdotool key Shift+Return
#24
xdotool key Shift+Return
#25
xdotool key Shift+Return
#26
xdotool key Shift+Return
#27
xdotool key Shift+Return
#28
xdotool key Shift+Return
#29
xdotool key Shift+Return
#32
xdotool key Return

#  Get result, kill xvfb
wait ${RPID}
RETV=$?
OUTPUT=$(cat output.txt | tr '\n' ' ')
if [ "${OUTPUT}" != '2 12 20 21 22 23 24 25 26 27 28 29 ' ]
then
    echo "Got: '${OUTPUT}' expected '2 12 20 21 22 23 24 25 26 27 28 29 '"
    exit 1
fi
echo ${RETV}
exit ${RETV}
