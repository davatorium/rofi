#!/usr/bin/env bash

if [ -z "${ROFI_OUTSIDE}" ]
then
    echo "run this script in rofi".
    exit
fi
echo -en "\x00no-custom\x1ftrue\n"
echo "${ROFI_RETV}"
