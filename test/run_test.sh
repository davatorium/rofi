#!/usr/bin/env bash

XPID=
FPID=
create_fake_x()
{
    export DISPLAY=":$1"
    echo -n "Starting fake X for display ${DISPLAY}: "
    Xvfb -nolisten tcp +extension XINERAMA +xinerama -screen 0 1280x1024x24 "${DISPLAY}" &>$2-server.log &
    XPID=$!
    echo "pid ${XPID}"
    sleep 1;
    if [ -x "$(which fluxbox 2>/dev/null)" ]; then
        echo -n "Starting fluxbox for display ${DISPLAY}: "
        timeout -k 30s 30s fluxbox &>$2-fluxbox.log &
        FPID=$!
        echo "pid ${FPID}"
        sleep 1
    fi
}

destroy_fake_x()
{
    if [ -n "${XPID}" ]
    then
        if [ -n "${FPID}" ]; then
            echo -n "Stopping fluxbox for display ${DISPLAY} (pid ${FPID}): "
            if kill "${FPID}" &>$1-kill-fluxbox.log; then
                echo -n " killed... "
                wait "${FPID}" &>$1-wait-fluxbox.log
                echo "stopped"
            else
                echo -n " failed to kill"
            fi
        fi
        echo -n "Stopping fake X for display ${DISPLAY} (pid ${XPID}): "
        if kill "${XPID}" &>$1-kill-X.log; then
            echo -n " killed... "
            wait "${XPID}" &>$1-wait-X.log
            echo "stopped"
        else
            echo -n " failed to kill"
        fi
    fi
}

if [ -n "$4" ]
then
    export PATH=$4:$PATH
fi

create_fake_x "$1" "$2"
"$3" "$5" &> "$2-test.log"
RES=$?

destroy_fake_x "$2"

exit "${RES}"
