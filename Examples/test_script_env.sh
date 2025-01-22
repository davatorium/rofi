#!/usr/bin/env bash

if [ -z "${ROFI_OUTSIDE}" ]
then
    echo "run this script in rofi".
    exit
fi

echo -en "\x00no-custom\x1ffalse\n"
echo -en "\x00data\x1fmonkey do, monkey did\n"
echo -en "\x00use-hot-keys\x1ftrue\n"
echo -en "${ROFI_RETV}\x00icon\x1ffirefox\x1finfo\x1ftest\n"

if [ -n "${ROFI_INFO}" ]
then
    echo "my info: ${ROFI_INFO} "
fi
if [ -n "${ROFI_DATA}" ]
then
    echo "my data: ${ROFI_DATA} "
fi
