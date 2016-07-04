## Open Board Viewer

AppVeyor Windows build: [![Build status](https://ci.appveyor.com/api/projects/status/avgn60acyn0cqyw4/branch/master?svg=true)](https://ci.appveyor.com/project/OpenBoardView/openboardview/branch/master) Travis Unix builds: [![Build Status](https://travis-ci.org/OpenBoardView/OpenBoardView.svg?branch=master)](https://travis-ci.org/OpenBoardView/OpenBoardView)

Software for viewing .brd files, intended as a drop-in replacement for the
"Test_Link" software.

[![Demo Video](https://github.com/chloridite/OpenBoardView/raw/master/asset/screenshot.png)](https://www.youtube.com/watch?v=1Pi5RGC-rJw)

### Installation

Downloadable an executable from the [Releases page](https://github.com/chloridite/OpenBoardView/releases)
and run it.

If you run into any issues starting the executable, you may need to install [OpenGL](https://www.opengl.org/wiki/Getting_Started).

### Usage

- Ctrl-O: Open a .brd file
- N: Search by power net
- C: Search by component name

### Building

To compile OpenBoardView you need **CMake**, a **build system** or **IDE** (e.g. Visual Studio or `make`) and a **C++ compiler** (e.g. MSVS, clang or GCC/G++).
You also need all the dependancies; currently they should all come included if you are using Visual Studio.

CMake is a **meta build system**. This means CMake generates a build system for you, such as Makefiles or a Visual Studio solution, which you then use to build OpenBoardView.

CMake includes a tool called CPack that can generate packages (e.g. `.zip` archives or `.msi` installers). To build an installer package you will need the [WiX Toolset](http://wixtoolset.org/releases/) installed and on your system PATH.

You don't usually want to make changes to the generated files, as the next time you run CMake they will probably be replaced! Instead if you want to change something, do it in the main source code files, in CMakeLists.txt, or (if it's something simple like a build option) just change an option using `ccmake` or cmake-gui.

It's also a good idea to keep the generated files (and the final build, too) separate from the development sources.

#### Using the command line

```
mkdir build
cd build
cmake ..
cmake --build . --target OpenBoardView --config Release
```
If you want to change settings, you can use `ccmake ..` or `cmake-gui ..` instead of `cmake ..`.

If you want to build the installer, use

```
cpack -G WIX -C Release
```

This will create the setup program `OpenBoardView-x.x.x-win32.exe` in your CMake build directory.

A zip file is as simple as

```
cpack -G ZIP -C Release
```

#### Visual Studio solution notes

If you decide to develop using Visual Studio's IDE, you may be confused by some of the "extra" projects CMake generates.

 - **ZERO_CHECK** - Building this project checks if the `CMakeLists.txt` files have changed. If they have it will ask you to "Reload" the solution. You don't normally have to build this manually as all other projects automatically depend on it.
 - **ALL_BUILD** - Building this will build everything in the solution.
 - **imgui** - This is a library used by OpenBoardView
 - **OpenBoardView** - This is the main project. You should probably set this as your "StartUp Project" too. _CMake 3.6+ will do this automatically._
 - **INSTALL** - Building this will install the build into CMAKE_INSTALL_PREFIX. This could require running as an administrator if CMAKE_INSTALL_PREFIX is outside your User folder.
 - **PACKAGE** - Building this will generate a setup program that can be used to fully install OpenBoardView using normal methods. You can choose what type of package is created with the `CPACK_GENERATOR` CMake option.
