project(OpenBoardView)

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}) 

cmake_minimum_required(VERSION 2.6.2)

set(INSTALL_LIBRARY_DIR lib)
set(INSTALL_ARCHIVE_DIR lib)
set(INSTALL_RUNTIME_DIR bin)
set(INSTALL_ASSET_DIR asset)
set(INSTALL_BUNDLE_DIR .)

set(CPACK_BUNDLE_NAME OpenBoardView)
set(CPACK_PACKAGE_NAME OpenBoardView)
set(CPACK_PACKAGE_VERSION 1)


set(CMAKE_MODULE_PATH 
        ${CMAKE_MODULE_PATH} 
        ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

add_definitions(-Wall)

#install(FILES asset/FiraSans-Medium.ttf DESTINATION ${INSTALL_ASSET_DIR})

set(ASSETS ${PROJECT_SOURCE_DIR}/asset/FiraSans-Medium.ttf)
set_source_files_properties(${ASSETS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

add_subdirectory(src)

INCLUDE(CPack)
