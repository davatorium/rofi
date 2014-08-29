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
DISPLAY=:202 xdotool key 'c'
sleep 0.2
DISPLAY=:202 xdotool key 'o'
sleep 0.2
DISPLAY=:202 xdotool key 'f'
sleep 0.2
DISPLAY=:202 xdotool key 'f'
sleep 0.2
DISPLAY=:202 xdotool key 'e'
sleep 0.2
DISPLAY=:202 xdotool key 'e'
sleep 0.2
DISPLAY=:202 xdotool key Return

#  Get result, kill xvfb
wait ${RPID}
RETV=$?
kill ${XPID}

sleep 1

if [ `cat output.txt` != 'coffee' ]
then
    exit 1
fi
exit ${RETV}
