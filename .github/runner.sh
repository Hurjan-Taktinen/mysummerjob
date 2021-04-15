#!/bin/sh
rsync -aq /app/ /copy --exclude=build

cd /copy
meson build
cd build
ninja
./tests
