#!/usr/bin/env bash

DIR=$1
FILE=$2
GIT=$(command -v git)
SED=$(command -v sed)

if [ -d "${DIR}/.git/" ] && [ -n "${GIT}" ]
then
    echo -n "#define GIT_VERSION \"" > "${FILE}.tmp"
    BRTG="$(${GIT} describe --tags --always --all | ${SED} -e 's:heads/::')"
    REV="$(${GIT} describe --tags --always --dirty| ${SED} -e 's:-g\([a-f0-9]\{7\}\):-git-\1:g')"
    echo -n "${REV} (${BRTG})" >> "${FILE}.tmp"
    echo "\"" >> "${FILE}.tmp"
else
    echo "#undef GIT_VERSION" > "${FILE}.tmp"
fi

if [ ! -f "${FILE}" ] || ! diff "${FILE}.tmp" "${FILE}" >/dev/null
then
    mv "${FILE}.tmp" "${FILE}"
else
    rm "${FILE}.tmp"
fi
