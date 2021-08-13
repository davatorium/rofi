#!/usr/bin/env bash

# wait till it is up, run rofi with error message
sleep 1
ulimit -c unlimited
echo -e -n "aap\nnoot\nmies" | rofi -dmenu -normal-window -multi-select > output.txt &
RPID=$!
sleep 4
xdotool getactivewindow windowsize 100% 100%
echo "Window resized"
# send enter.
sleep 1
xdotool key 'Down'
sleep 0.4
xdotool key Shift+Return
xdotool key Shift+Return
xdotool key Return

# Get result, kill xvfb
wait "${RPID}"
RETV=$?
if [ "${RETV}" == "139" ]
then
    echo "thread apply all bt" | gdb rofi core.*
fi

OUTPUT=$( tr '\n' ' ' < output.txt )
if [ "${OUTPUT}" != 'noot mies ' ]
then
    echo "Got: '${OUTPUT}' expected 'noot mies '"
    exit 1
fi
echo "${RETV}"
exit "${RETV}"
