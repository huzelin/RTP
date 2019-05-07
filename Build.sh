#!/bin/sh
if [ ! -d build ]; then
  mkdir build
fi
set -x

LWP=`pwd`
pushd build

cmake ../
make -j 2
