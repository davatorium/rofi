#!/usr/bin/env sh

DIR=$1
FILE=$2
GIT=$(which git)

if [ -d "${DIR}/.git/" ] && [ -n "${GIT}" ]
then
    echo -n "#define GIT_VERSION \""  > ${FILE}.tmp
    REV="$(${GIT} describe --tags --always --dirty) ($(${GIT} describe --tags --always --all | sed -e 's:heads/::'))"
    echo -n "${REV}" >> ${FILE}.tmp
    echo "\"" >> ${FILE}.tmp
else
    echo "#undef GIT_VERSION" > ${FILE}.tmp
fi

if [ ! -f ${FILE} ] || ! diff ${FILE}.tmp ${FILE} > /dev/null
then
    mv ${FILE}.tmp ${FILE}
else
    rm ${FILE}.tmp
fi
