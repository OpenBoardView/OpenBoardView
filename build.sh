#!/bin/sh

TPUT_B="$(tput bold)"
TPUT_0="$(tput sgr0)"

color() {
  color="$1"
  text="$2"
  echo "$TPUT_B$(tput setaf ${color})${text}$TPUT_0"
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
    THREADS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || getconf NPROCESSORS_ONLN 2>/dev/null || echo 1)"
fi
STRCOMPILE="$(color 2 Compiling)"
RECOMPILE=false
COMPILEDIR="release_build"
COMPILEFLAGS="-DCMAKE_INSTALL_PREFIX="
export DESTDIR="$(cd "$(dirname "$0")" && pwd)"
BUILDTYPE="$(color 6 release)"


for arg in "$@"; do
  case $arg in
    --help)
      helpMsg
      exit
    ;;
    --debug)
      COMPILEDIR="debug_build"
      COMPILEFLAGS="$COMPILEFLAGS -DCMAKE_BUILD_TYPE=DEBUG"
      BUILDTYPE="$(color 1 debug)"
    ;;
    --recompile)
      STRCOMPILE="$(color 5 Recompiling)"
      RECOMPILE=true
    ;;
    *) # pass other arguments to CMAKE
      COMPILEFLAGS="$COMPILEFLAGS $arg"
  esac
done

if [ "$CROSS" = "mingw64" ]; then
  COMPILEFLAGS="$COMPILEFLAGS -DCMAKE_TOOLCHAIN_FILE=../Toolchain-mingw64.cmake"
fi

if [ $THREADS -lt 1 ]; then
  color 1 "Unable to detect number of threads, using 1 thread."
  THREADS=1
fi
if [ "$RECOMPILE" = true ]; then
  rm -rf $COMPILEDIR
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
