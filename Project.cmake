### Project.cmake ###
#
# Project metadata and packaging information
#
# Run by CMakeLists.txt
#

project("OpenBoardView" LANGUAGES CXX VERSION "0.3.0")
set(PRETTY_NAME "Open Board View")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Software for viewing .brd files, intended as a drop-in replacement for the
\"Test_Link\" software.")

# Owner
set(CPACK_PACKAGE_VENDOR "Chloridite")

# Ensure CPack uses the same data as CMake
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

# Store metadata in the build dir (for CI packaging and other automation)
file(WRITE "${PROJECT_BINARY_DIR}/PROJECT_NAME" "${PROJECT_NAME}")
file(WRITE "${PROJECT_BINARY_DIR}/PROJECT_VERSION" "${PROJECT_VERSION}")

## Install locations
# Allow user to easily discover the install prefix
mark_as_advanced(CLEAR FORCE CMAKE_INSTALL_PREFIX)

if(WIN32)
	# Name the install directory; goes under CMAKE_INSTALL_PREFIX
	set(CPACK_PACKAGE_INSTALL_DIRECTORY
		"${CPACK_PACKAGE_NAME} - ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}"
	CACHE PATH "Directory created in install prefix to contain instalation")

	# Install binaries in the root of the install directory
	set(INSTALL_BIN_DIR "/" CACHE PATH "Where to place binaries in the instalation")
else()
	set(INSTALL_BIN_DIR "bin" CACHE PATH "Where to place binaries in the instalation")
endif()



# Used for Windows and OSX shortcuts; ${PROJECT_NAME} is a console command,
# "Open Board View" is the pretty text
set(CPACK_PACKAGE_EXECUTABLES ${PROJECT_NAME} ${PRETTY_NAME})

if(WIN32)
	# WiX MD5 Hash, used for checking whether to upgrade an installed package,
	# or install a new one
	#		hash generated manually using `echo "openboardview" | md5sum` on unix
	#   dashes should be added to form the following pattern:
	#   8-4-4-4-12    XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
	set(CPACK_WIX_UPGRADE_GUID "e10554b8-a9ce-9f67-8a09-68fbc9474a7a"
			CACHE STRING "Dash separated MD5 hash, used by WiX to perform updates.")
	mark_as_advanced(FORCE CPACK_WIX_UPGRADE_GUID)

	# Which packages should we generate when installing "package" target
	# or running CPack without specifying a generator
	set(CPACK_GENERATOR  WIX ZIP CACHE STRING "List of generators to build packages with")
endif()

## Text files (like LICENCE.txt) are added from asset/CMakeLists.txt
