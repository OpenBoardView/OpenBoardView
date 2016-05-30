project(OpenBoardView)

cmake_minimum_required(VERSION 2.6.2)

set(INSTALL_DIR ${PROJECT_SOURCE_DIR})
set(INSTALL_LIBRARY_DIR ${INSTALL_DIR}/lib)
set(INSTALL_ARCHIVE_DIR ${INSTALL_DIR}/lib)
set(INSTALL_RUNTIME_DIR ${INSTALL_DIR}/bin)
set(INSTALL_LUA_PACKAGE_DIR ${INSTALL_DIR}/script/package)

set(CMAKE_MODULE_PATH 
        ${CMAKE_MODULE_PATH} 
        ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

add_definitions(-Wall)

add_subdirectory(src)
