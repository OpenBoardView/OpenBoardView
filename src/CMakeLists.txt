## COMMON ##
option(ENABLE_GL1 "Build OpenGL 1 renderer." ON)
option(ENABLE_GL3 "Build OpenGL 3 renderer." ON)
option(ENABLE_GLES2 "Configure OpenGL 3 renderer to be OpenGL ES 2.0 compatible." OFF)

if(NOT APPLE AND NOT WIN32 OR MINGW)
	find_package(PkgConfig REQUIRED)
endif()

## utf8.h ##
execute_process(
	COMMAND git submodule update --init src/utf8
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

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
set(GLAD_API "gl=3.0,gles2=2.0" CACHE STRING "GL versions" FORCE) # Build for OpenGL 3.0 and OpenGLES 2
set(GLAD_EXTENSIONS "GL_EXT_texture_compression_s3tc,GL_OES_element_index_uint" CACHE STRING "Gl exts" FORCE) # S3TC used to load texture for pins — delete when removing support. The other is used by GLES2.
set(GLAD_NO_LOADER ON CACHE BOOL "Disable loader" FORCE) # We're using SDL2 loader
add_subdirectory(glad)

## SDL2 ##
find_package(SDL2 REQUIRED CONFIG)
add_definitions(-DENABLE_SDL2)

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
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples/imgui_impl_opengl2.cpp" input)
string(REPLACE "OpenGL/gl.h" "glad/glad.h" input "${input}")
string(REPLACE "GL/gl.h" "glad/glad.h" input "${input}")
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples/imgui_impl_opengl2.cpp" "${input}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/imgui
	${GLAD_INCLUDE_DIRS}
)

set(SOURCES
	imgui/imgui.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_widgets.cpp
	imgui/examples/imgui_impl_sdl.cpp)


if(ENABLE_GL1)
	LIST(APPEND SOURCES
		imgui/examples/imgui_impl_opengl2.cpp
	)
endif()
if(ENABLE_GL3)
	LIST(APPEND SOURCES
		imgui/examples/imgui_impl_opengl3.cpp
	)
endif()

add_library(imgui STATIC ${SOURCES})
target_link_libraries(imgui
	${GLAD_LIBRARIES}
)
target_link_libraries(imgui
	SDL2::SDL2
)

set(IMGUI_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/imgui ${CMAKE_CURRENT_SOURCE_DIR}/imgui/examples)


#install(TARGETS imgui DESTINATION ${INSTALL_ARCHIVE_DIR}) # No need to install a static lib

## OpenBoardView ##
add_subdirectory(openboardview)
