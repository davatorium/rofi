#!/usr/bin/env bash

# wait till it is up, run rofi with error message
sleep 1;
echo -e -n "aap\nnoot\nmies" | rofi -dmenu  > output.txt & 
RPID=$!

# send enter.
sleep 5;
xdotool key 'Down' 
sleep 0.4
xdotool key Shift+Return
xdotool key Return

#  Get result, kill xvfb
wait ${RPID}
RETV=$?
OUTPUT=$(cat output.txt | tr '\n' ' ')
if [ "${OUTPUT}" != 'noot mies ' ]
then
    echo "Got: '${OUTPUT}' expected 'noot mies '"
    exit 1
fi
echo ${RETV}
exit ${RETV}
