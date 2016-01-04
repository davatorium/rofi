#!/usr/bin/env bash

echo -en "nooty\naap\nnoot\nmies" | rofi -fuzzy -dmenu   > output.txt  &
RPID=$!

# send enter.
sleep 5;
xdotool type 'nty'
sleep 0.4
xdotool key Return

#  Get result, kill xvfb
wait ${RPID}
RETV=$?
OUTPUT=$(cat output.txt)
if [ "${OUTPUT}" != 'nooty' ]
then
    echo "Got: '${OUTPUT}' expected 'nooty'"
    exit 1
fi

exit ${RETV}
