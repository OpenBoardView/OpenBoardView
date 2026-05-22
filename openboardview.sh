#!/bin/sh

# Get the absolute path to the directory containing the script
SCRIPT_DIR=$(dirname "$(realpath "$0")")
cd ${SCRIPT_DIR}

if [ ! -x "./bin/openboardview" ]; then ./build.sh; fi
export LD_LIBRARY_PATH=lib
export DYLD_LIBRARY_PATH=lib
./bin/openboardview $@
