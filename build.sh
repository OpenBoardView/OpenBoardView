#!/usr/bin/env bash
export REVISION=`git rev-list HEAD --count`
PROJECT="$(tput bold ; tput setaf 3)OpenBoardView $(tput setaf 2)r$REVISION$(tput sgr0)"
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
if [ "$1" == "--help" ]; then
  echo "Usage: $(tput bold ; tput setaf 4)$0$(tput sgr0) [--$(tput bold ; tput setaf 5)recompile$(tput sgr0)] [--$(tput bold ; tput setaf 1)debug$(tput sgr0)] — Build $PROJECT"
  echo "          --$(tput bold ; tput setaf 5)recompile$(tput sgr0)   — Delete $(tput bold ; tput setaf 6)\$COMPILEDIR$(tput sgr0) (release_build or debug_build with --$(tput bold ; tput setaf 1)debug$(tput sgr0)) before compiling $PROJECT again" 
  echo "          --$(tput bold ; tput setaf 1)debug$(tput sgr0)       — Make a $(tput bold ; tput setaf 1)debug$(tput sgr0) build"
  echo ""
  echo "All extra parameters are passed to cmake."
  exit
fi
STRCOMPILE="$(tput bold ; tput setaf 2)Compiling$(tput sgr0)"
COMPILEDIR="release_build"
COMPILEFLAGS=""
BUILDTYPE="$(tput bold ; tput setaf 6)release$(tput sgr0)"
SCRIPT_ARGC=1 # number of arguments eaten by this script
if [ "$ARG_LENGTH" -gt 0 -a "$1" == "--debug" -o "$2" == "--debug" ]; then
  COMPILEDIR="debug_build"
  COMPILEFLAGS="$COMPILEFLAGS -DCMAKE_BUILD_TYPE=DEBUG"
  BUILDTYPE="$(tput bold ; tput setaf 1)debug$(tput sgr0)"
  SCRIPT_ARGC=$((SCRIPT_ARGC+1))
fi
if [ "$ARG_LENGTH" -gt 0 -a "$1" == "--recompile" -o "$2" == "--recompile" ]; then
  STRCOMPILE="$(tput bold ; tput setaf 5)Recompiling$(tput sgr0)"
  rm -rf $COMPILEDIR
  SCRIPT_ARGC=$((SCRIPT_ARGC+1))
fi
COMPILEFLAGS="$COMPILEFLAGS ${@:${SCRIPT_ARGC}}" # pass other arguments to CMAKE
if [ $THREADS -lt 1 ]; then
  echo "$(tput bold ; tput setaf 1)Unable to detect number of threads, using 1 thread.$(tput sgr0)"
  THREADS=1
fi
if [ ! -d $COMPILEDIR ]; then
  mkdir $COMPILEDIR
fi
cd $COMPILEDIR
STRTHREADS="threads"
if [ $THREADS -eq 1 ]; then
  STRTHREADS="thread"
fi

# Now compile the source code and install it in server's directory
echo "$STRCOMPILE $PROJECT using $(tput bold ; tput setaf 4)$THREADS$(tput sgr0) $STRTHREADS ($BUILDTYPE build)"
echo "Extra flags passed to CMake: $COMPILEFLAGS"
cmake $COMPILEFLAGS ..
[[ "$?" != "0" ]] && echo "$(tput bold ; tput setaf 1)CMAKE FAILED$(tput sgr0)" && exit 1
if `echo "$COMPILEFLAGS" | grep -q "DEBUG"`; then
  make -j$THREADS install
  [[ "$?" != "0" ]] && echo "$(tput bold ; tput setaf 1)MAKE INSTALL FAILED$(tput sgr0)" && exit 1
else
  make -j$THREADS install/strip
  [[ "$?" != "0" ]] && echo "$(tput bold ; tput setaf 1)MAKE INSTALL/STRIP FAILED$(tput sgr0)" && exit 1
fi

case "$(uname -s)" in
  *Darwin*)
    # Generate DMG
    make package
    [[ "$?" != "0" ]] && echo "$(tput bold ; tput setaf 1)MAKE PACKAGE FAILED$(tput sgr0)" && exit 1
    ;;
  *)
    # Give right execution permissions to executables
    cd ../bin
    for i in openboardview; do chmod +x $i; done

    ;;
esac
