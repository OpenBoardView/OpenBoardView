### lib/CMakeLists.txt ###
#
#

## imgui:
add_library(imgui imgui/imgui.cpp imgui/imgui_draw.cpp)
target_link_libraries(imgui ${PLATFORM_LIBS})

## utf8: no need to ass_lib since it is only a header file
