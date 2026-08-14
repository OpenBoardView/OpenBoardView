#!/bin/sh

# Get the absolute path to the directory containing the script
SCRIPT_DIR=$(dirname "$(realpath "$0")")
SCRIPT="${SCRIPT_DIR}"/bin/openboardview

if [ ! -x "${SCRIPT}" ]; then
    echo "${SCRIPT} not found"
    echp "Consider going to ${SCRIPT_DIR} and running ./build.sh;"
fi

export LD_LIBRARY_PATH="$SCRIPT_DIR/lib"
export DYLD_LIBRARY_PATH="$SCRIPT_DIR/lib"
${SCRIPT} $@

