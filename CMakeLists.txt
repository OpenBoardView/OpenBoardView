cmake_policy(SET CMP0048 OLD)
set(PROJECT_VERSION "R7.2")
project(OpenBoardView)
cmake_minimum_required(VERSION 2.8.9)

# Colorized output. Checks TERM to exclude Windows command prompt
if(DEFINED ENV{TERM})
	string(ASCII 27 Esc)
	set(CGreenB "${Esc}[1;32m")
	set(CReset "${Esc}[m")
endif()

# Build info
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
message("${PROJECT_NAME} release:${CGreenB} ${OBV_BUILD}${CReset}")

# GCC version check
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9")
		message(FATAL_ERROR "GCC version needs to be >= 4.9")
	endif()
endif()

# Install in project directory
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})
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
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_definitions(-DOBV_BUILD="${OBV_BUILD}") # Build info

add_subdirectory(asset)
add_subdirectory(src)

set(CPACK_BUNDLE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
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
	set(CPACK_GENERATOR DEB CACHE STRING "List of generators to build packages with")
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "zlib1g, libgtk-3-0, sqlite3, libsdl2-2.0-0, libfontconfig1")
	set(CPACK_DEBIAN_PACKAGE_SECTION "Electronics")
	set(CPACK_DEBIAN_PACKAGE_VERSION "1${PROJECT_VERSION}")
elseif(WIN32)
	set(CPACK_GENERATOR ZIP CACHE STRING "List of generators to build packages with")
	set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
endif()
INCLUDE(CPack)
