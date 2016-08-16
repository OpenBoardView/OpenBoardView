### lib/CMakeLists.txt ###
#
#

## Platform and system libraries
include(PlatformLibs.cmake)

## imgui:
execute_process(
	COMMAND git submodule update --init imgui
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(imgui imgui/imgui.cpp imgui/imgui_draw.cpp)
target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
target_link_libraries(imgui ${PLATFORM_LIBS})
target_compile_definitions(imgui PUBLIC "-DImDrawIdx=unsigned int") # short is not enough for us (long causes core dumps)
set(LIBS ${LIBS} imgui)


## glfw ##
# Fallback to submodule if not found by platform_libs.cmake
if((NOT USE_SYSTEM_GLFW) OR (NOT GLFW3_FOUND))
	message("\n  Using included GLFW submodule\n")

	execute_process(
		COMMAND git submodule update --init glfw
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)

	# Set sane defaults
	if(NOT CONFIGURED)
		set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
		set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
		set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
		set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
	endif()
	# Hide GLFW 'spam' from CMake GUI
	mark_as_advanced(FORCE LIB_SUFFIX GLFW_DOCUMENT_INTERNALS GLFW_BUILD_DOCS GLFW_BUILD_TESTS GLFW_BUILD_EXAMPLES)

	# Actually add glfw to build
	add_subdirectory(glfw)

	# GLFW needs a special flag for shared lib linking
	if(BUILD_SHARED_LIBS)
	target_compile_definitions(glfw INTERFACE -DGLFW_DLL)
	endif()

	set(LIBS ${LIBS} glfw)
endif()


## utf8: no need to add_lib since it is only a header file
execute_process(
	COMMAND git submodule update --init utf8
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set(INCLUDE_DIRS ${INCLUDE_DIRS} "${CMAKE_CURRENT_SOURCE_DIR}/utf8")

## Export ${LIBS}, ${PLATFORM_LIBS}, ${INCLUDE_DIRS} and ${CFLAGS} for use with main target
set(LIBS ${LIBS} PARENT_SCOPE)
set(PLATFORM_LIBS ${PLATFORM_LIBS} PARENT_SCOPE)
set(INCLUDE_DIRS ${INCLUDE_DIRS} PARENT_SCOPE)
set(CFLAGS ${CFLAGS} PARENT_SCOPE)
