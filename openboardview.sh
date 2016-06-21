#!/bin/sh
if [ ! -x "./bin/openboardview" ]; then ./build.sh; fi
export LD_LIBRARY_PATH=lib
export DYLD_LIBRARY_PATH=lib
./bin/openboardview "$1"
