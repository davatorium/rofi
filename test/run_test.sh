#!/usr/bin/env bash

XPID=
FPID=
function create_fake_x ( )
{
    export DISPLAY=":$1"
    echo "Starting fake X: ${DISPLAY}"
    Xvfb -screen 0 1280x1024x24  ${DISPLAY} &
    XPID=$!
    sleep 1;
    timeout -k 30s 30s fluxbox &
    FPID=$!
    sleep 1
}

function destroy_fake_x ( )
{
    if [ -n "${XPID}" ]
    then
        echo "Stopping fake X: ${XPID} - ${FPID}"
        kill ${FPID}
        echo " wait flux"
        wait ${FPID}
        kill ${XPID}
        echo "wait x"
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
