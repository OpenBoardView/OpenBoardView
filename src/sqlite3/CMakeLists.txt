PROJECT(sqlite3)
cmake_minimum_required(VERSION 2.6.2)

# See https://github.com/android-ndk/ndk/issues/332
if(ANDROID)
	add_definitions(-D_FILE_OFFSET_BITS=32)
endif()

add_library(sqlite3 STATIC sqlite3.c)
if(NOT WIN32 AND NOT ANDROID)
	target_link_libraries(sqlite3 pthread)
endif()
#install(TARGETS sqlite3 DESTINATION ${INSTALL_ARCHIVE_DIR}) # No need to install a static lib

set(SQLITE3_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}" PARENT_SCOPE)
set(SQLITE3_LIBRARIES "sqlite3" PARENT_SCOPE)
