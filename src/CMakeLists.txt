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

## gl3w ##
execute_process(
	COMMAND git submodule update --init src/gl3w
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
# Download include/GL/glcorearb.h & generate src/gl3w.c and include/GL/gl3w.h
execute_process(
	COMMAND ./gl3w_gen.py
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/gl3w
)
add_subdirectory(gl3w)

## imgui ##
execute_process(
	COMMAND git submodule update --init src/imgui
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
add_definitions("-DImDrawIdx=unsigned int") # short is not enough for us
add_library(imgui SHARED imgui/imgui.cpp imgui/imgui_draw.cpp)
target_link_libraries(imgui ${PLATFORM_TARGET_LIBS})
install(TARGETS imgui DESTINATION ${INSTALL_LIBRARY_DIR})

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

add_subdirectory(openboardview)
