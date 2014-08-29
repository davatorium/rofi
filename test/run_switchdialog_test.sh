#!/usr/bin/env bash

# Create fake X
Xvfb :200 &
XPID=$!

# wait till it is up, run rofi with error message
sleep 1 && ./rofi -rnow  -display :200 & 
RPID=$!

# send enter.
sleep 5;
DISPLAY=:200 xdotool key 'shift+slash' 
sleep 0.4
DISPLAY=:200 xdotool key 'shift+slash'
sleep 0.4
DISPLAY=:200 xdotool key 'shift+slash'
sleep 0.4
DISPLAY=:200 xdotool key Escape

#  Get result, kill xvfb
wait ${RPID}
RETV=$?
kill ${XPID}
exit ${RETV}
