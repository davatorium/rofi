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
sleep 1;
kill ${TPID}
if pgrep -u $USER xterm
then
    kill ${XPID}
    kill ${RPID}
    exit 1
fi
#  Get result, kill xvfb
wait ${RPID}
RETV=$?
exit ${RETV}
