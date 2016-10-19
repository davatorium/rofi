#!/usr/bin/env bash

TOP_DIR=$1

rofi -h -config ${TOP_DIR}/doc/test_xr.txt | grep -v "Version"> help-output.txt

if ! diff  help-output.txt ${TOP_DIR}/doc/help-output.txt > /dev/null
then
    echo "Help output does not match."
    exit 1;
fi

exit ${RETV}
