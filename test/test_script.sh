#!/usr/bin/env bash

if [ -z "$1" ]
then
    echo "aap"
    echo "noot"
    echo "mies"
else
    echo $1 > output.txt
fi
