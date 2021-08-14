#!/usr/bin/env bash

TOP_DIR=$1

xrdb -retain -load "${TOP_DIR}/doc/old-theme-convert-input.theme"
rofi -config "${TOP_DIR}/doc/old-theme-convert-input.theme" -dump-theme | grep -v "Rofi version" > temp.txt

if ! diff temp.txt "${TOP_DIR}/doc/old-theme-convert-output.rasi" >/dev/null
then
    echo "Convert default theme failed"
    diff temp.txt "${TOP_DIR}/doc/old-theme-convert-output.rasi"
    exit 1
fi

exit "${RETV}"
