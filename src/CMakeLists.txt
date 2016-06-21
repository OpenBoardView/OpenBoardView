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
add_library(gl3w SHARED gl3w/src/gl3w.c)
target_include_directories(gl3w PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gl3w/include)
target_link_libraries(gl3w ${PLATFORM_TARGET_LIBS})
install(TARGETS gl3w DESTINATION ${INSTALL_LIBRARY_DIR})

## imgui ##
add_library(imgui SHARED imgui/imgui.cpp imgui/imgui_draw.cpp)
target_link_libraries(imgui ${PLATFORM_TARGET_LIBS})
install(TARGETS imgui DESTINATION ${INSTALL_LIBRARY_DIR})

## JSON11 ##
add_library(json11 SHARED json11/json11.cpp)
#target_include_directories(json11 ${CMAKE_CURRENT_SOURCE_DIR}/json11)
target_compile_options(json11
  PUBLIC -std=c++11
  PRIVATE -fno-rtti -fno-exceptions -Wall -Wextra -Werror)

install(TARGETS json11 DESTINATION ${INSTALL_LIBRARY_DIR})

add_subdirectory(openboardview)
