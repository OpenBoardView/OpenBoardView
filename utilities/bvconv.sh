#!/bin/sh

if [ $# -ne 1 ]; then
echo "This tool converts the .bv board format file in to a format that can be read by the boardviewer"
echo
echo "BV files are Microsoft Access DB format files, so you need to install the mdb-tools package and"
echo " have 'mdb-export' in your path ( https://github.com/brianb/mdbtools )."
echo
echo "This tool will dump the contents of the file in to a decodable plain text format."
echo
echo "Usage: $0 input.bv > output.bvr"
exit 1
fi

echo "BRRAW_FORMAT_1"
echo "[Layout]"
mdb-export "$1" Layout 
echo "[Nail]"
mdb-export "$1" Nail
echo "[Pin]"
mdb-export "$1" Pin

