## COMMON ##
option(ENABLE_GL1 "Build OpenGL 1 renderer." ON)
option(ENABLE_GL3 "Build OpenGL 3 renderer." ON)
option(ENABLE_GLES2 "Configure OpenGL 3 renderer to be OpenGL ES 2.0 compatible." OFF)

if(NOT APPLE AND NOT WIN32 OR MINGW)
	find_package(PkgConfig REQUIRED)
endif()

## utf8.h ##
find_path(UTF8_INCLUDE_DIR
	NAMES "utf8.h"
	PATH_SUFFIXES "utf8")
if(NOT UTF8_INCLUDE_DIR)
	execute_process(
		COMMAND git submodule update --init src/utf8
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		)
	set(UTF8_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/utf8)
endif()

## zlib ##
find_package(ZLIB)
if(NOT ZLIB_FOUND)
	execute_process(
		COMMAND git submodule update --init src/zlib
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)
	execute_process( # Dirty workaround to prevent zlib from building examples
		COMMAND sed "/^# Example binaries$/,$d" -i src/zlib/CMakeLists.txt
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)

	set(SKIP_INSTALL_FILES YES)
	set(SKIP_INSTALL_HEADERS NO)
	add_subdirectory(zlib)

	if(WIN32)
		set(ZLIB_LIBRARIES zlibstatic CACHE INTERNAL "zlib library")
	else()
		set(ZLIB_LIBRARIES z CACHE INTERNAL "zlib library")
	endif()
	set(ZLIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/zlib ${CMAKE_BINARY_DIR}/src/zlib)
endif()

## SQLite3 ##
find_package(SQLite3)
if(NOT SQLITE3_FOUND)
	add_subdirectory(sqlite3)
endif()

## glad ##
execute_process(
	COMMAND git submodule update --init src/glad
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
set(GLAD_PROFILE "compatibility" CACHE STRING "GL Profile" FORCE) # Using OpenGL Compatibility profile (for enabling use of fixed-function pipeline)
set(GLAD_API "gl=3.2,gles2=2.0" CACHE STRING "GL versions" FORCE) # Build for OpenGL 3.0 and OpenGLES 2
set(GLAD_EXTENSIONS "GL_EXT_texture_compression_s3tc,GL_OES_element_index_uint" CACHE STRING "Gl exts" FORCE) # S3TC used to load texture for pins — delete when removing support. The other is used by GLES2.
set(GLAD_NO_LOADER ON CACHE BOOL "Disable loader" FORCE) # We're using SDL2 loader
set(GLAD_REPRODUCIBLE ON CACHE BOOL "Reproducible build" FORCE)
add_subdirectory(glad)

## SDL2 ##
find_package(SDL2 REQUIRED CONFIG)
add_definitions(-DENABLE_SDL2)

# Create namespace targets for system with SDL2 older than 2.0.12
if(NOT TARGET SDL2::SDL2)
	add_library(SDL2::SDL2 SHARED IMPORTED)
	find_library(SDL2_LIBRARY SDL2)
	set_target_properties(SDL2::SDL2 PROPERTIES
		IMPORTED_LOCATION ${SDL2_LIBRARY}
		INTERFACE_INCLUDE_DIRECTORIES ${SDL2_INCLUDE_DIRS})
	find_library(SDL2MAIN_LIBRARY SDL2main)
	if(SDL2MAIN_LIBRARY)
		add_library(SDL2::SDL2main STATIC IMPORTED)
		set_target_properties(SDL2::SDL2main PROPERTIES
			IMPORTED_LOCATION ${SDL2MAIN_LIBRARY}
			INTERFACE_INCLUDE_DIRECTORIES ${SDL2_INCLUDE_DIRS})
	else()
		add_library(SDL2::SDL2main INTERFACE IMPORTED)
	endif()
endif()

## imgui ##
# note: in the future there may be integrated CMake support into imgui
# see: https://github.com/ocornut/imgui/pull/1713
# for now do it manually, after glad and SDL2 because we use the includes for the sdl_opengl examples
execute_process(
	COMMAND git submodule update --init src/imgui
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
add_definitions("-DImDrawIdx=unsigned int") # short is not enough for us
add_definitions("-DIMGUI_IMPL_OPENGL_LOADER_GLAD") # We use glad
# Configure GL3 renderer to be GLES2 compatible if GLES2 is enabled
if(ENABLE_GLES2)
    add_definitions("-DIMGUI_IMPL_OPENGL_ES2")
endif()

# workaround for OpenGL include for OpenGL2, need to be glad rather than gl itself
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl2.cpp" input)
string(REPLACE "OpenGL/gl.h" "glad/glad.h" input "${input}")
string(REPLACE "GL/gl.h" "glad/glad.h" input "${input}")
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl2.cpp" "${input}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/imgui
	${GLAD_INCLUDE_DIRS}
)

set(SOURCES
	imgui/imgui.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_tables.cpp
	imgui/imgui_widgets.cpp
	imgui/misc/cpp/imgui_stdlib.cpp
	imgui/backends/imgui_impl_sdl2.cpp)


if(ENABLE_GL1)
	LIST(APPEND SOURCES
		imgui/backends/imgui_impl_opengl2.cpp
	)
endif()
if(ENABLE_GL3)
	LIST(APPEND SOURCES
		imgui/backends/imgui_impl_opengl3.cpp
	)
endif()

add_library(imgui STATIC ${SOURCES})
target_link_libraries(imgui
	${GLAD_LIBRARIES}
)
if(MINGW)
target_link_libraries(imgui
	SDL2::SDL2-static
)
else()
target_link_libraries(imgui
	SDL2::SDL2
)
endif()

set(IMGUI_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/imgui ${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples)


#install(TARGETS imgui DESTINATION ${INSTALL_ARCHIVE_DIR}) # No need to install a static lib

## filesystem ##
message(CHECK_START "Checking for std::filesystem")
## GCC 9 and Clang 9 include filesystem in the standard library. Apple is of course messing with us and AppleClang 11.4 is Clang 9.0
if(((CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU") AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9.0))
	#OR (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 11.4))
	message(CHECK_PASS "available")
	set(CMAKE_CXX_STANDARD 17)
	set(FILESYSTEM_LIBRARIES)
	add_definitions(-DWITH_STD_FILESYSTEM)
# Clang 8 requires explicit linking with lc++fs. AppleClang 11.0 is Clang 8.0
elseif((CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 8.0))
	#OR (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 11.0))
	message(CHECK_PASS "available")
	set(CMAKE_CXX_STANDARD 17)
	set(FILESYSTEM_LIBRARIES lc++fs)
	add_definitions(-DWITH_STD_FILESYSTEM)
# GCC 8 requires explicit linking with stdc++fs
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 8.0)
	message(CHECK_PASS "available")
	set(CMAKE_CXX_STANDARD 17)
	set(FILESYSTEM_LIBRARIES stdc++fs)
	add_definitions(-DWITH_STD_FILESYSTEM)
else() # Other compiler/version are unsupported, use external lib
	message(CHECK_FAIL "not available, using ghc::filesystem")
	execute_process(
		COMMAND git submodule update --init src/filesystem
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)
	add_subdirectory(filesystem)
	set(FILESYSTEM_LIBRARIES ghc_filesystem)
endif()

## stb ##
if(NOT APPLE AND NOT WIN32 OR MINGW)
	pkg_check_modules(STB stb)
endif()

if(NOT STB_FOUND)
	find_path(STB_INCLUDE_DIRS
		NAMES "stb_image.h"
		HINTS ${STB_INCLUDE_DIRS}
		PATH_SUFFIXES "stb"
	)
endif()

if(STB_INCLUDE_DIRS)
	add_definitions(${STB_CFLAGS_OTHER})
	include_directories(${STB_INCLUDE_DIRS})
else()
	execute_process(
		COMMAND git submodule update --init src/stb
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/stb)
endif()

add_definitions(-DSTBI_FAILURE_USERMSG)

## mpc - Micro Parser Combinators ##
execute_process(
	COMMAND git submodule update --init src/mpc
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_library(mpc STATIC "mpc/mpc.c")


## OpenBoardView ##
add_subdirectory(openboardview)
