#!/usr/bin/env bash

# wait till it is up, run rofi with error message
sleep 1;
xterm &
XPID=$!
sleep 1;
echo -e -n "aap\nnoot\nmies" | rofi -modi window -show window > output.txt & 
RPID=$!

# send enter.
sleep 5;
xdotool type 'xterm' 
sleep 0.4
xdotool key Return
sleep 1;
xdotool key Ctrl+d
wait ${XPID}
#  Get result, kill xvfb
wait ${RPID}
RETV=$?
exit ${RETV}
