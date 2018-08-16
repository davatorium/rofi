#!/usr/bin/env bash

# wait till it is up, run rofi with error message
sleep 1;
xterm -T MonkeySee sh &
XPID=$!
echo "Started MonkeySee xterm: pid ${XPID}"
sleep 1;
xterm -T TermUnwanted sh &
TPID=$!
echo "Started TermUnwanted xterm: pid ${TPID}"
sleep 1;
rofi -modi window -show window > output.txt &
RPID=$!
echo "Started rofi: pid ${RPID}"

# send enter.
sleep 5;
xdotool type 'MonkeySee'
sleep 0.4
xdotool key Return
sleep 1;
xdotool key Ctrl+d
sleep 1;

echo -n "Killing TermUnwanted: "
if kill ${TPID}; then
    echo "done"
    wait ${TPID}
fi

if ps -q ${XPID} # pgrep -u $USER xterm
then
    echo "Found remaining xterms: $(pgrep -u $USER xterm)"
    kill ${XPID}
fi
if ps -q ${RPID}
then
    echo "Rofi still running"
    kill ${RPID}
    exit 1
fi
#  Get result, kill xvfb
wait ${RPID}
RETV=$?
exit ${RETV}
