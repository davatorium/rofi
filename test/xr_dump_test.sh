#!/usr/bin/env bash

xrdb -retain -load ../doc/test_xr.txt
rofi -dump-xresources > temp.txt

if ! diff temp.txt ../doc/test_xr.txt > /dev/null
then
    echo "Dump xresources does not match."
    exit 1;
fi

exit ${RETV}
