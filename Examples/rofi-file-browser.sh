#!/usr/bin/env bash

EDITOR=xdg-open

CUR_DIR=$PWD

PREV_LOC_FILE=~/.rofi_fb_prevloc

# Read last location, otherwise we default to PWD.
if [ -f "${PREV_LOC_FILE}" ]
then
    CUR_DIR=$(cat "${PREV_LOC_FILE}") 
fi 

# Handle argument.
if [ -n "$@" ]
then
    CUR_DIR="${CUR_DIR}/$@"
fi

# If argument is no directory.
if [ ! -d "${CUR_DIR}" ]
then
    if [ -x "${CUR_DIR}" ]
    then
        coproc ( "${CUR_DIR}" &  > /dev/null 2>&1 )
        exec 1>&-
        exit;
    elif [ -f "${CUR_DIR}" ]
    then
        coproc ( ${EDITOR} "${CUR_DIR}" & > /dev/null  2>&1 )
        exit;
    fi
    exit;
fi

# process current dir.
if [ -n "${CUR_DIR}" ]
then
    CUR_DIR=$(readlink -e "${CUR_DIR}")
    echo "${CUR_DIR}" > "${PREV_LOC_FILE}"
    pushd "${CUR_DIR}" >/dev/null
fi

echo ".."
ls
