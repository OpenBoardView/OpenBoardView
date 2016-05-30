## gl3w ##
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	find_library(coreFoundation CoreFoundation)
endif()
add_library(gl3w SHARED gl3w/src/gl3w.c)
target_include_directories(gl3w PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gl3w/include)
target_link_libraries(gl3w ${coreFoundation})
install(TARGETS gl3w DESTINATION ${INSTALL_LIBRARY_DIR})

## imgui ##

add_library(imgui SHARED imgui/imgui.cpp imgui/imgui_draw.cpp)
install(TARGETS imgui DESTINATION ${INSTALL_LIBRARY_DIR})


add_subdirectory(openboardview)
