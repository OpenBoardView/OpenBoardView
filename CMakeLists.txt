project(OpenBoardView)
cmake_minimum_required(VERSION 2.8.9)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9")
		message(FATAL_ERROR "GCC version needs to be >= 4.9")
	endif()
endif()

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}) 
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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_subdirectory(asset)
add_subdirectory(src)

set(CPACK_BUNDLE_NAME OpenBoardView)
set(CPACK_PACKAGE_NAME OpenBoardView)
set(CPACK_PACKAGE_VERSION R3)
set(CPACK_OUTPUT_FILE_PREFIX  "${CMAKE_CURRENT_SOURCE_DIR}")

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
endif()

INCLUDE(CPack)
