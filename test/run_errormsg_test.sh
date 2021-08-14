#!/usr/bin/env bash

# wait till it is up, run rofi with error message
sleep 1 && rofi -e "Printing error message" &
RPID=$!

# send enter.
sleep 5 && xdotool key Return

# Get result, kill xvfb
wait "${RPID}"
RETV=$?
exit "${RETV}"
