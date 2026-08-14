#!/bin/sh

# Get the absolute path to the directory containing the script
SCRIPT_DIR=$(dirname "$(realpath "$0")")
BINARY="${SCRIPT_DIR}"/bin/openboardview

if [ ! -x "${BINARY}" ]; then
    echo "ERROR: ${BINARY} : file does not exist or is not readable"
    echo "Consider going to ${SCRIPT_DIR} and running ./build.sh;"
    exit 127
fi

export LD_LIBRARY_PATH="$SCRIPT_DIR/lib"
export DYLD_LIBRARY_PATH="$SCRIPT_DIR/lib"
${BINARY} $@

