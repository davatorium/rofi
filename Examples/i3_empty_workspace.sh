#!/usr/bin/env bash

MAX_DESKTOPS=20

WORKSPACES=$(seq -s '\n' 1 1 ${MAX_DESKTOPS})

EMPTY_WORKSPACE=$( (i3-msg -t get_workspaces | tr ',' '\n' | grep num | awk -F:  '{print int($2)}' ; \
            echo -e ${WORKSPACES} ) | sort -n | uniq -u | head -n 1)

i3-msg workspace ${EMPTY_WORKSPACE}
