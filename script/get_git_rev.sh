#!/usr/bin/env bash

DIR=$1
FILE=$2

if [ -d "${DIR}/.git/" ]
then
    echo -n "#define GIT_VERSION \""  > ${FILE}.tmp
    REV=$(git log -1 --pretty=tformat:%h)
    echo -n "${REV}" >> ${FILE}.tmp
    echo "\"" >> ${FILE}.tmp
else
    echo "#undef GIT_VERSION" > ${FILE}.tmp
fi

if ! diff ${FILE}.tmp ${FILE} > /dev/null 
then
    mv ${FILE}.tmp ${FILE}
else
    rm ${FILE}.tmp
fi
