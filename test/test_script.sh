#!/usr/bin/env bash

if [ -z "$1" ]
then
    echo "aap"
    echo "noot"
    echo -ne "mies\0meta\x1fzoom\n"
else
    echo "$1" > output.txt
fi
