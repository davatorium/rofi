#!/usr/bin/env bash

# wait till it is up, run rofi with error message
sleep 1 && rofi -show run  -display :201 &
RPID=$!

# send enter.
sleep 5;
xdotool key 'shift+slash' 
sleep 0.4
xdotool key 'shift+slash'
sleep 0.4
xdotool key 'shift+slash'
sleep 0.4
xdotool key Escape

#  Get result, kill xvfb
wait ${RPID}
RETV=$?

sleep 1

exit ${RETV}
