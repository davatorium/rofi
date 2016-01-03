#!/usr/bin/env bash

TOP_DIR=$1

rofi -dump-xresources -config ${TOP_DIR}/doc/test_xr.txt > temp.txt

if ! diff temp.txt ${TOP_DIR}/doc/test_xr.txt > /dev/null
then
    echo "Dump xresources does not match."
    exit 1;
fi

exit ${RETV}
