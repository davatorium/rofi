#!/usr/bin/env bash

cat /dev/null | rofi -dmenu &
RPID=$!
sleep 4
xdotool key Return
#  Get result, kill xvfb
wait ${RPID}
RETV=$?

if [ ${RETV} -eq 1 ]
then
    exit 0
else
    exit 1
fi
