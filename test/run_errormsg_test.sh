#!/usr/bin/env bash

# Create fake X
Xvfb :200 &
XPID=$!

# wait till it is up, run rofi with error message
sleep 1 && ./rofi -e "Printing error message" -display :200 & 
RPID=$!

# send enter.
sleep 5 && DISPLAY=:200 xdotool key Return

#  Get result, kill xvfb
wait ${RPID}
RETV=$?
kill ${XPID}
sleep 1
exit ${RETV}
