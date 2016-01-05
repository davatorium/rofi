#!/usr/bin/env bash

XPID=
FPID=
function create_fake_x ( )
{
    export DISPLAY=":$1"
    echo "Starting fake X: ${DISPLAY}"
    Xvfb ${DISPLAY} &
    XPID=$!
    fluxbox &
    FPID=$!
    sleep 1
}

function destroy_fake_x ( )
{
    if [ -n "${XPID}" ]
    then
        echo "Stopping fake X: ${XPID} - ${FPID}"
        kill ${FPID}
        wait ${FPID}
        kill ${XPID}
        wait ${XPID}
    fi
}

if [ -n "$3"  ]
then
    export PATH=$3:$PATH
fi

create_fake_x "$1"
echo "$DISPLAY"
$2 $4
RES=$?

destroy_fake_x

exit ${RES}
