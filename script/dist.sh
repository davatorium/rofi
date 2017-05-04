#! /bin/sh

NAME=`mesonintrospect ${MESON_BUILD_ROOT} --projectinfo | tr ',[' '\n\n' | sed -n 1,/subprojects/p | grep name | cut -d'"' -f4`
VERSION=`mesonintrospect ${MESON_BUILD_ROOT} --projectinfo | tr ',[' '\n\n' | sed -n 1,/subprojects/p | grep version | cut -d'"' -f4`
PREFIX=${NAME}-${VERSION}
TAR=${MESON_BUILD_ROOT}/${PREFIX}.tar

rm -f ${TAR} ${TAR}.xz
git archive --prefix=${PREFIX}/ --format=tar HEAD > ${TAR}
(
    git submodule | \
    while read commit path ref; do
        (
            cd ${path}
            git archive --prefix=${PREFIX}/${path}/ --format=tar ${commit} > tmp.tar
            tar -Af ${TAR} tmp.tar
            rm -f tmp.tar
        )
    done
)
xz ${TAR}
