#!/usr/bin/env bash

XPID=
DISPLAY=":0"
function create_fake_x ( )
{
    DISPLAY=":$1"
    echo "Starting fake X: ${DISPLAY}"
    Xvfb ${DISPLAY} &
    XPID=$!
    sleep 1
}

function destroy_fake_x ( )
{
    if [ -n "${XPID}" ]
    then
        echo "Stopping fake X: ${XPID}"
        kill ${XPID}
        wait ${XPID}
    fi
}

if [ -n "$3"  ]
then
    PATH=$3:$PATH
fi

create_fake_x "$1"
$2
RES=$?

destroy_fake_x

exit ${RES}
