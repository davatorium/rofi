#!/usr/bin/env bash

# Create fake X
Xvfb :201 &
XPID=$!

# wait till it is up, run rofi with error message
sleep 1 && ./rofi -rnow  -display :201 & 
RPID=$!

# send enter.
sleep 5;
DISPLAY=:201 xdotool key 'shift+slash' 
sleep 0.4
DISPLAY=:201 xdotool key 'shift+slash'
sleep 0.4
DISPLAY=:201 xdotool key 'shift+slash'
sleep 0.4
DISPLAY=:201 xdotool key Escape

#  Get result, kill xvfb
wait ${RPID}
RETV=$?
kill ${XPID}

sleep 1

exit ${RETV}
