#!/usr/bin/env bash

if [ "$*" = "quit" ]
then
    exit 0
fi

if [ "$@" ]
then
    # Override the previously set prompt.
    echo -en "\0theme\x1felement-text { background-color: "$@";}\n"
    echo "red"
    echo "lightgreen"
    echo "lightblue"
    echo "lightyellow"
    echo "quit"
else
    echo -en "\x00prompt\x1ftesting\n"
    echo -en "\0urgent\x1f0,2\n"
    echo -en "\0active\x1f1\n"
    echo -en "\0message\x1fSpecial <b>bold</b>message\n"

    echo "red"
    echo "lightgreen"
    echo "lightblue"
    echo "lightyellow"
    echo "quit"
fi
