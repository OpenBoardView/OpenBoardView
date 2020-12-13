PROJECT(SQlite)
cmake_minimum_required(VERSION 2.8.12)

add_library(SQLite3 STATIC sqlite3.c)
target_include_directories(SQLite3 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
if(NOT WIN32)
	target_link_libraries(SQLite3 pthread)
endif()
#install(TARGETS sqlite3 DESTINATION ${INSTALL_ARCHIVE_DIR}) # No need to install a static lib

add_library(SQLite::SQLite3 ALIAS SQLite3)
set(SQLite3_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}" PARENT_SCOPE)
set(SQLite3_LIBRARIES "sqlite3" PARENT_SCOPE)
