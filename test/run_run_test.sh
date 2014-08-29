#!/usr/bin/env bash

# Create fake X
Xvfb :203 &
XPID=$!

# wait till it is up, run rofi with error message
sleep 1;
./rofi -rnow -display :203  & 
RPID=$!

# send enter.
sleep 5;
DISPLAY=:203 xdotool key 't' 
sleep 0.4
DISPLAY=:203 xdotool key 'r'
sleep 0.4
DISPLAY=:203 xdotool key 'u'
sleep 0.4
DISPLAY=:203 xdotool key 'e'
sleep 0.4
DISPLAY=:203 xdotool key Return

#  Get result, kill xvfb
wait ${RPID}
RETV=$?
kill ${XPID}

sleep 1

exit ${RETV}
