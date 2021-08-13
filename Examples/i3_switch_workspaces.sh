#!/usr/bin/env bash

if [ -z $@ ]
then
gen_workspaces()
{
    i3-msg -t get_workspaces | tr ',' '\n' | grep "name" | sed 's/"name":"\(.*\)"/\1/g' | sort -n
}

echo empty; gen_workspaces
else
    WORKSPACE=$@

    if [ "${WORKSPACE}" = "empty" ]
    then
        i3_empty_workspace.sh >/dev/null
    elif [ -n "${WORKSPACE}" ]
    then
        i3-msg workspace "${WORKSPACE}" >/dev/null
    fi
fi
