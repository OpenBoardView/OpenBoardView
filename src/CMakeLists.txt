## gl3w ##
add_library(gl3w SHARED gl3w/src/gl3w.c)
target_include_directories(gl3w PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gl3w/include)
install(TARGETS gl3w DESTINATION ${INSTALL_LIBRARY_DIR})

## imgui ##

add_library(imgui SHARED imgui/imgui.cpp imgui/imgui_draw.cpp)
install(TARGETS imgui DESTINATION ${INSTALL_LIBRARY_DIR})


add_subdirectory(openboardview)
