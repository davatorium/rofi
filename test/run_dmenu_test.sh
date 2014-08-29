#!/usr/bin/env bash

# Create fake X
Xvfb :202 &
XPID=$!

# wait till it is up, run rofi with error message
sleep 1;
echo -e -n "aap\nnoot\nmies" | ./rofi -dmenu -display :202 > output.txt & 
RPID=$!

# send enter.
sleep 5;
DISPLAY=:202 xdotool key 'Down' 
sleep 0.4
DISPLAY=:202 xdotool key 'Down'
sleep 0.4
DISPLAY=:202 xdotool key Return

#  Get result, kill xvfb
wait ${RPID}
RETV=$?
kill ${XPID}

sleep 1

if [ `cat output.txt` != 'aap' ]
then
    exit 1
fi
exit ${RETV}
