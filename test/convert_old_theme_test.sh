#!/usr/bin/env bash

TOP_DIR=$1

rofi -config ${TOP_DIR}/doc/old-theme-convert-input.theme -dump-theme  > temp.txt

if ! diff temp.txt ${TOP_DIR}/doc/old-theme-convert-output.rasi > /dev/null
then
    echo "Dump default theme does not match."
    diff temp.txt ${TOP_DIR}/doc/old-theme-convert-output.rasi
    exit 1;
fi

exit ${RETV}
