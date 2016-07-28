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


## utf8: no need to add_lib since it is only a header file
execute_process(
	COMMAND git submodule update --init utf8
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set(INCLUDE_DIRS ${INCLUDE_DIRS} "${CMAKE_CURRENT_SOURCE_DIR}/utf8")

## nowide
execute_process(
	COMMAND git submodule update --init nowide
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_subdirectory(nowide)
set(LIBS ${LIBS} nowide-static)
set(INCLUDE_DIRS ${INCLUDE_DIRS} "${CMAKE_CURRENT_SOURCE_DIR}/nowide")

## Export ${LIBS}, ${PLATFORM_LIBS} and ${INCLUDE_DIRS}
set(LIBS ${LIBS} PARENT_SCOPE)
set(PLATFORM_LIBS ${PLATFORM_LIBS} PARENT_SCOPE)
set(INCLUDE_DIRS ${INCLUDE_DIRS} PARENT_SCOPE)
