#!/bin/sh
if [ ! -x "./bin/openboardview" ]; then ./build.sh; fi
LD_LIBRARY_PATH=lib ./bin/openboardview
