#!/usr/bin/env bash

rofi -rnow   & 
RPID=$!

# send enter.
sleep 5;
xdotool key 't' 
sleep 0.4
xdotool key 'r'
sleep 0.4
xdotool key 'u'
sleep 0.4
xdotool key 'e'
sleep 0.4
xdotool key Return

#  Get result, kill xvfb
wait ${RPID}
RETV=$?

exit ${RETV}
