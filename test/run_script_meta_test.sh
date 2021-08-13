#!/usr/bin/env bash

SP=$(readlink -f "$0")
DIR=$(dirname "$SP")
echo "$DIR/test_script.sh"
# wait till it is up, run rofi with error message
sleep 1
rofi -modi "custom:$DIR/test_script.sh" -show custom &
RPID=$!

# send enter.
sleep 5
xdotool key 'z'
sleep 0.4
xdotool key Return

# Get result, kill xvfb
wait "${RPID}"
RETV=$?
OUTPUT=$( tr '\n' ' ' < output.txt )
echo "${OUTPUT}"
if [ "${OUTPUT}" != 'mies ' ]
then
    exit 1
fi
exit "${RETV}"
