#!/usr/bin/env bash

export ROFI_PNG_OUTPUT=out.png
rofi -modi run -key-run Alt+F2  &
RPID=$!

# send enter.
sleep 3;
xdotool key Alt+F2 
sleep 3;
xdotool key 't' 
sleep 0.4
xdotool key 'r'
sleep 0.4
xdotool key 'u'
sleep 0.4
xdotool key Return
sleep 1;
kill -SIGINT ${RPID}
#  Get result, kill xvfb
wait ${RPID}
RETV=$?

exit ${RETV}
