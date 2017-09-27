#!/usr/bin/env bash

echo -en "\0prompt\x1ftesting\n"
echo -en "\0urgent\x1f0,2\n"
echo -en "\0active\x1f1\n"
echo -en "\0message\x1fSpecial message\n"

echo "aap"
echo "noot"
echo "mies"
echo "testing"
if [ -n "$@" ]
then
    echo "$@"
fi
