#!/bin/sh
rsync -aq /app/ /copy --exclude=build

cd /copy
CC=gcc-10 CXX=g++-10 meson build
cd build
ninja
./tests
