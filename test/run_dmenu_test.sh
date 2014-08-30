#!/usr/bin/env bash

# wait till it is up, run rofi with error message
sleep 1;
echo -e -n "aap\nnoot\nmies" | rofi -dmenu  > output.txt & 
RPID=$!

# send enter.
sleep 5;
xdotool key 'Down' 
sleep 0.4
xdotool key 'Down'
sleep 0.4
xdotool key Return

#  Get result, kill xvfb
wait ${RPID}
RETV=$?

if [ `cat output.txt` != 'mies' ]
then
    exit 1
fi
exit ${RETV}
