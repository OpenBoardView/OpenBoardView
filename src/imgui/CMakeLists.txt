set(IMGUI_LIBRARY imgui CACHE INTERNAL "imgui library")

#        include_directories(${LUA_INCLUDE_DIRS} ${MYSQL_INCLUDE_DIRS})

        add_library(${IMGUI_LIBRARY} SHARED imgui.cpp imgui_draw.cpp)
#        target_link_libraries(${LUASQL_MYSQL_LIBRARY} ${LUA_LIBRARY} ${MYSQL_LIBRARIES})

        install(TARGETS
                        ${IMGUI_LIBRARY}
                LIBRARY DESTINATION
                        ${INSTALL_LIBRARY_DIR}
                ARCHIVE DESTINATION
                        ${INSTALL_ARCHIVE_DIR})
