#!/usr/bin/env bash

# wait till it is up, run rofi with error message
sleep 1;
xterm -T MonkeySee sh &
XPID=$!
sleep 1;
xterm -T TermUnwanted sh &
TPID=$!
sleep 1;
rofi -modi window -show window > output.txt &
RPID=$!

# send enter.
sleep 5;
xdotool type 'MonkeySee'
sleep 0.4
xdotool key Return
sleep 1;
xdotool key Ctrl+d
wait ${XPID}
kill ${TPID}
#  Get result, kill xvfb
wait ${RPID}
RETV=$?
exit ${RETV}
