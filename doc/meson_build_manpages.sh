#!/usr/bin/env bash

## Did not get this working in meson directly.
## not via generator or custom_target.


pushd "${MESON_BUILD_ROOT}"

for a in $@
do
  go-md2man -in "$a" -out ${a%.markdown}
done
