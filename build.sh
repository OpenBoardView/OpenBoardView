#!/bin/sh

color() {
  color="$1"
  text="$2"
  echo "$(tput bold; tput setaf ${color})${text}$(tput sgr0)"
}

helpMsg() {
  cat << EOH
Usage: $(color 4 ${0}) [--$(color 5 recompile)] [--$(color 1 debug)] — Build $PROJECT
          --$(color 5 recompile)   — Delete $(color 6 \$COMPILEDIR) (release_build or debug_build with --$(color 1 debug)) before compiling $PROJECT again
          --$(color 1 debug)       — Make a $(color 1 debug) build

All extra parameters are passed to cmake.
Environment variables:
          CROSS         — Set to "mingw64" to cross-compile for Windows
EOH
}

PROJECT="$(color 3 OpenBoardView)"
if [ -z $THREADS ]; then
    THREADS=1
    case "$(uname -s)" in
      *Darwin*|*BSD*)
        THREADS=`sysctl -n hw.ncpu`
        ;;
      *)
        if [ -r "/proc/cpuinfo" ]; then
            THREADS=`cat /proc/cpuinfo | grep processor | wc -l`
        fi
        ;;
    esac
fi
ARG_LENGTH=$#
if [ "$1" = "--help" ]; then
  helpMsg
  exit
fi
STRCOMPILE="$(color 2 Compiling)"
COMPILEDIR="release_build"
COMPILEFLAGS="-DCMAKE_INSTALL_PREFIX="
export DESTDIR="$(cd "$(dirname "$0")" && pwd)"
BUILDTYPE="$(color 6 release)"
SCRIPT_ARGC=1 # number of arguments eaten by this script
if [ "$ARG_LENGTH" -gt 0 -a "$1" = "--debug" -o "$2" = "--debug" ]; then
  COMPILEDIR="debug_build"
  COMPILEFLAGS="$COMPILEFLAGS -DCMAKE_BUILD_TYPE=DEBUG"
  BUILDTYPE="$(color 1 debug)"
  SCRIPT_ARGC=$((SCRIPT_ARGC+1))
fi
if [ "$ARG_LENGTH" -gt 0 -a "$1" = "--recompile" -o "$2" = "--recompile" ]; then
  STRCOMPILE="$(color 5 Recompiling)"
  rm -rf $COMPILEDIR
  SCRIPT_ARGC=$((SCRIPT_ARGC+1))
fi
if [ "$CROSS" = "mingw64" ]; then
  COMPILEFLAGS="$COMPILEFLAGS -DCMAKE_TOOLCHAIN_FILE=../Toolchain-mingw64.cmake"
fi
SUBSTRING=$(echo $@ | cut -d ' ' -f ${SCRIPT_ARGC}-)
COMPILEFLAGS="$COMPILEFLAGS ${SUBSTRING}" # pass other arguments to CMAKE
if [ $THREADS -lt 1 ]; then
  color 1 "Unable to detect number of threads, using 1 thread."
  THREADS=1
fi
if [ ! -d $COMPILEDIR ]; then
  mkdir $COMPILEDIR
fi
LASTDIR=$PWD
cd $COMPILEDIR
STRTHREADS="threads"
if [ $THREADS -eq 1 ]; then
  STRTHREADS="thread"
fi

# Now compile the source code and install it in server's directory
echo "$STRCOMPILE $PROJECT using $(color 4 $THREADS) $STRTHREADS ($BUILDTYPE build)"
echo "Extra flags passed to CMake: $COMPILEFLAGS"
cmake $COMPILEFLAGS ..
[ "$?" != "0" ] && color 1 "CMAKE FAILED" && exit 1
if `echo "$COMPILEFLAGS" | grep -q "DEBUG"`; then
  make -j$THREADS install
  [ "$?" != "0" ] && color 1 "MAKE INSTALL FAILED" && exit 1
else
  make -j$THREADS install/strip
  [ "$?" != "0" ] && color 1 "MAKE INSTALL/STRIP FAILED" && exit 1
fi

case "$(uname -s)" in
  *Darwin*)
    # Generate DMG
    make package
    [ "$?" != "0" ] && color 1 "MAKE PACKAGE FAILED" && exit 1
    ;;
  *)
    # Give right execution permissions to executables
    cd $LASTDIR
    cd bin
    for i in openboardview; do chmod +x $i; done

    ;;
esac

cd $LASTDIR
exit 0
