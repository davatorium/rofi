#!/usr/bin/env bash

TOP_DIR=$1

rofi -no-config -dump-theme | grep -v "Rofi version" > temp.txt

if ! diff temp.txt "${TOP_DIR}/doc/default_theme.rasi" >/dev/null
then
    echo "Dump default theme does not match."
    diff temp.txt "${TOP_DIR}/doc/default_theme.rasi"
    exit 1
fi

exit "${RETV}"
