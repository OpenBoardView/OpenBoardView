## COMMON ##
if(NOT APPLE AND NOT WIN32 OR MINGW)
	find_package(PkgConfig REQUIRED)
endif()

## imgui ##
execute_process(
	COMMAND git submodule update --init src/imgui
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
add_definitions("-DImDrawIdx=unsigned int") # short is not enough for us
add_library(imgui STATIC imgui/imgui.cpp imgui/imgui_draw.cpp)
#install(TARGETS imgui DESTINATION ${INSTALL_ARCHIVE_DIR}) # No need to install a static lib

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
set(GLAD_API "gl=3.2,gles2=2.0" CACHE STRING "GL versions" FORCE) # Build for OpenGL 3.2 and OpenGLES 2
set(GLAD_EXTENSIONS "GL_EXT_texture_compression_s3tc,GL_OES_element_index_uint" CACHE STRING "Gl exts" FORCE) # S3TC used to load texture for pins — delete when removing support. The other is used by GLES2.
set(GLAD_NO_LOADER ON CACHE BOOL "Disable loader" FORCE) # We're using SDL2 loader
add_subdirectory(glad)

## SDL2 ##
if(APPLE)
	find_package(SDL2 REQUIRED)
	set(SDL2_INCLUDE_DIRS "${SDL2_INCLUDE_DIR}")
	set(SDL2_LIBRARIES "${SDL2_LIBRARY}")
elseif(WIN32 AND NOT MINGW)
	set(SDL2_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/SDL2/include")
	set(SDL2_LIBRARIES "SDL2main" "SDL2")
	set(SDL2_LIBRARY_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/SDL2/lib/x86")
else()
	pkg_check_modules(SDL2 REQUIRED sdl2)
endif()
add_definitions(-DENABLE_SDL2)

add_subdirectory(openboardview)
