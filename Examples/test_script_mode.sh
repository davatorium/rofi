#!/usr/bin/env bash

if [ x"$1" = x"quit" ]
then
    exit 0
fi

if [ "$4" ]
then
    exit 0
elif [ "$3" ]
then
    echo -en "\x00prompt\x1fsubsubsubmenu $3 $2 $1\n"
    echo "quit"
elif [ "$2" ]
then
    echo -en "\x00prompt\x1fsubsubmenu $2 $1\n"
    for a in {20..25}
    do
        echo "$a"
    done
    echo "quit"
elif [ "$1" ]
then
    echo -en "\x00prompt\x1fsubmenu $1\n"
    for a in {1..10}
    do
        echo "$a"
    done
    echo "quit"
else
    echo -en "\x00prompt\x1ftesting\n"
    echo -en "\0urgent\x1f0,2\n"
    echo -en "\0active\x1f1\n"
    echo -en "\0markup-rows\x1ftrue\n"
    echo -en "\0message\x1fSpecial <b>bold</b>message\n"

    echo -en "aap\0icon\x1ffolder\n"
    echo "noot"
    echo "mies"
    echo "testing"
    echo "<b>Bold</b>"
    echo "quit"
fi
