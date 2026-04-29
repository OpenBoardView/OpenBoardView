cmake_minimum_required(VERSION 3.11)

# Set OSX target version, before calling project() (inside version.cmake). FORCE is needed when project() is called within an included file
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version" FORCE)

# Set a default build type like suggested in official blog https://www.kitware.com/cmake-and-the-default-build-type/
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to Release as none was specified.")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

# According to CMake doc, project() must be called in the top-level file, not in an included file
# Also since CMake 3.0 version should be defined using project() too
# VERSION must be up to 4 groups of digits separated by a period.
# major.90 are alpha releases
# major.95 are beta releases
project(OpenBoardView VERSION 9.95.2)
option(FLATPAK "Build for Flatpak" OFF)

# Include our project variables
include("version.cmake")

# Colorized output. Checks TERM to exclude Windows command prompt
if(DEFINED ENV{TERM})
	string(ASCII 27 Esc)
	set(CGreenB "${Esc}[1;32m")
	set(CReset "${Esc}[m")
endif()

# Build info
if (NOT DEFINED OBV_BUILD)
	if(EXISTS "${PROJECT_SOURCE_DIR}/.git/")
		execute_process(
			COMMAND git rev-list HEAD --count
			OUTPUT_VARIABLE GIT_REVISION
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		execute_process(
			COMMAND git name-rev --name-only HEAD
			OUTPUT_VARIABLE GIT_BRANCH
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		execute_process(
			COMMAND git config branch.${GIT_BRANCH}.remote
			OUTPUT_VARIABLE GIT_REMOTE
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		execute_process(
			COMMAND git config remote.${GIT_REMOTE}.url
			OUTPUT_VARIABLE GIT_URL
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		string(REGEX REPLACE ".*/(.+)/.+$" "\\1" GIT_REPO "${GIT_URL}") # Extract the user from a Github repo URL
		set(OBV_BUILD "R${GIT_REVISION} ${GIT_REPO}/${GIT_BRANCH}")
	else()
		set(OBV_BUILD "${PROJECT_VERSION}")
	endif()
endif()
message("${PROJECT_NAME} release:${CGreenB} ${OBV_BUILD}${CReset}")

# GCC version check
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.8")
		message(FATAL_ERROR "GCC version needs to be >= 4.8")
	endif()
endif()

# Extra CMake modules (Find*.cmake) are in CMakeModules/
set(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

set(INSTALL_RUNTIME_DIR bin)
set(INSTALL_ARCHIVE_DIR lib)

if(APPLE)
	set(BUNDLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/src/openboardview/openboardview.app")
#	set(LIBRARY_OUTPUT_PATH "${BUNDLE_OUTPUT_PATH}/Contents/MacOS") # We don't want to put static lib inside the bundle
	set(INSTALL_LIBRARY_DIR "openboardview.app/Contents/MacOS")
	set(INSTALL_BUNDLE_DIR .)
else()
	set(INSTALL_LIBRARY_DIR lib)
endif()

# Warning with CMake 3.0 on OS X
set(CMAKE_MACOSX_RPATH ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
if(NOT MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -Werror=format-security")
endif()
# Workaround type bug of kSecKeyUsageCritical in SecCertificate.h for Xcode 8.2.1 SDK 10.12
if(APPLE)
	execute_process(
		COMMAND xcrun --show-sdk-version
		OUTPUT_VARIABLE OSX_SDK_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(OSX_SDK_VERSION VERSION_EQUAL 10.12)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-narrowing")
	endif()
endif()
set(CMAKE_CXX_STANDARD 11)

# Inject variables into version.h
configure_file("${PROJECT_SOURCE_DIR}/src/openboardview/version.in.h"
			   "${PROJECT_BINARY_DIR}/include/version.h"
			   @ONLY ESCAPE_QUOTES)
include_directories("${PROJECT_BINARY_DIR}/include")

add_subdirectory(asset)
add_subdirectory(src)

set(CPACK_BUNDLE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_CONTACT "piernov <piernov@piernov.org>")
set(CPACK_OUTPUT_FILE_PREFIX  "${CMAKE_CURRENT_SOURCE_DIR}")
if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "DEBUG")
	set(CPACK_STRIP_FILES 1)
endif()

if(APPLE)
	set(CPACK_GENERATOR DragNDrop CACHE STRING "List of generators to build packages with")
	set(CPACK_DMG_VOLUME_NAME "OpenBoardView")
#	set(CPACK_DMG_FORMAT "UDRW") # Read-write DMG
	set(CPACK_DMG_BACKGROUND_IMAGE ${CMAKE_CURRENT_SOURCE_DIR}/asset/screenshot.png)
	set(CPACK_DMG_DS_STORE ${CMAKE_CURRENT_SOURCE_DIR}/asset/DMG.DS_Store)

	set(DIRS "\${LIBRARY_OUTPUT_PATH};/Library/Frameworks")

	INSTALL(CODE "
		set(BU_CHMOD_BUNDLE_ITEMS ON)
		file(GLOB_RECURSE LIBS
			\"${BUNDLE_OUTPUT_PATH}/Contents/MacOS/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
		include(BundleUtilities)
		fixup_bundle(\"${BUNDLE_OUTPUT_PATH}\" \"\${LIBS}\" \"${DIRS}\")
	")
elseif(UNIX)
	set(CPACK_GENERATOR DEB RPM CACHE STRING "List of generators to build packages with")
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "zlib1g, libgtk-3-0, sqlite3, libsdl2-2.0-0, libfontconfig1")
	set(CPACK_DEBIAN_PACKAGE_SECTION "electronics")
	set(CPACK_DEBIAN_PACKAGE_RELEASE 1)
	set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
	set(CPACK_RPM_PACKAGE_GROUP "Applications/Engineering")
	set(CPACK_RPM_PACKAGE_REQUIRES "gtk3") # Probably not working
	set(CPACK_RPM_PACKAGE_LICENSE "${PROJECT_LICENSE}")
	set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
	set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
	    "/usr/share/appdata"
	    "/usr/share/applications"
	    "/usr/share/icons"
	    "/usr/share/icons/hicolor"
	    "/usr/share/icons/hicolor/scalable"
	    "/usr/share/icons/hicolor/scalable/apps"
	    "/usr/share/mime"
	    "/usr/share/mime/packages"
	    "/usr/share/metainfo")
elseif(WIN32)
	set(CPACK_GENERATOR ZIP CACHE STRING "List of generators to build packages with")
	set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
endif()
INCLUDE(CPack)
