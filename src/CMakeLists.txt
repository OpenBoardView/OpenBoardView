## COMMON ##
if(APPLE)
	find_library(coreFoundation CoreFoundation)
    set(PLATFORM_TARGET_LIBS ${coreFoundation})
elseif(MINGW) # Should this check for WIN32?
    set(PLATFORM_TARGET_LIBS libopengl32.a imm32.lib)
elseif(UNIX)
    set(PLATFORM_TARGET_LIBS ) # Emply for now
else()
    message(WARNING
            "Make sure you set `PLATFORM_TARGET_LIBS` for your platform. \
          See:  ${CMAKE_CURRENT_LIST_FILE}")
endif()

## imgui ##
execute_process(
	COMMAND git submodule update --init src/imgui
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
add_definitions("-DImDrawIdx=unsigned int") # short is not enough for us
add_library(imgui STATIC imgui/imgui.cpp imgui/imgui_draw.cpp)
target_link_libraries(imgui ${PLATFORM_TARGET_LIBS})
#install(TARGETS imgui DESTINATION ${INSTALL_ARCHIVE_DIR}) # No need to install a static lib

## utf8.h ##
execute_process(
	COMMAND git submodule update --init src/utf8
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

## zlib ##
find_package(ZLIB)
if(NOT ZLIB_FOUND)
	set(SKIP_INSTALL_FILES YES)
	set(SKIP_INSTALL_HEADERS YES)
	add_subdirectory(zlib)

	set(ZLIB_LIBRARIES z CACHE INTERNAL "zlib library")
	set(ZLIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/zlib)
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

add_subdirectory(openboardview)
