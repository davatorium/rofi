#!/usr/bin/env bash

XPID=
FPID=
function create_fake_x ( )
{
    export DISPLAY=":$1"
    echo "Starting fake X: ${DISPLAY}"
    Xvfb +extension XINERAMA +xinerama -screen 0 1280x1024x24 -screen 1 800x600x24  ${DISPLAY} &>test-x-logs/xserver-:$1.log &
    XPID=$!
    sleep 1;
    timeout -k 30s 30s fluxbox &>test-x-logs/fluxbox-:$1.log &
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

if [ -n "$3" ]
then
    export PATH=$3:$PATH
fi

create_fake_x "$1"
"$2" "$4"
RES=$?

destroy_fake_x

exit ${RES}
