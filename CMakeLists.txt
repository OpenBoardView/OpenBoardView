### CMakeLists.txt ###
#
# Run by CMake to generate project solution or makefiles
#

cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)

# For when we set property CXX_STANDARD.
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable solution folders (in IDEs)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Name the solution folder for targets like INSTALL, PACKAGE, ZERO_CHECK
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "Meta Targets")

## MetaData
project(OpenBoardView CXX)
include(CMakeMetaData.txt)

# Compile flags and definitions

# Set UNICODE so strings work properly on Windows
add_definitions(-DUNICODE -D_UNICODE)

## Install locations
set(INSTALL_RUNTIME_DIR bin CACHE PATH "The location to install binary executables")
# TODO: asset, lib, include, etc

## System libraries
set(PLATFORM_LIBS
	#imm32.lib
	d3d9.lib
	kernel32.lib
	user32.lib
	gdi32.lib
	winspool.lib
	comdlg32.lib
	advapi32.lib
	shell32.lib
	ole32.lib
	oleaut32.lib
	uuid.lib
	odbc32.lib
	odbccp32.lib
)


# Include directories (anywhere there's a .h file)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui
	${CMAKE_CURRENT_SOURCE_DIR}/lib/utf8
)

## Add library targets
add_subdirectory(lib)

# Add libs to a solution folder
set_target_properties(imgui
	PROPERTIES FOLDER Libraries)

## Add assets
add_subdirectory(asset)

## Add main project
add_subdirectory(src)

# New feature in CMake 3.6 lets us define the Visual Studio StartUp Project
set_property(
	DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

## Set install destinations
#if shared
#install(TARGETS imgui utf8
#	DESTINATION ${INSTALL_LIBRARY_DIR})
#endif

# Enable packaging installers
include(CPack)

## Final warnings/errors/messages

# Check PROPERTY VS_STARTUP_PROJECT works if generating for VS
if (${CMAKE_VERSION} VERSION_LESS "3.6" AND     # feature introduced in 3.6
    ${CMAKE_GENERATOR} MATCHES "Visual Studio") # we only care if generating for VS
	message("\n")
	message(WARNING
		"Visual Studio's \"StartUp Project\" cannot be set automatically by CMake ${CMAKE_VERSION}.\n"
		"Try upgrading to CMake 3.6 or use Visual Studio's solution explorer to manually set "
		"\"${PROJECT_NAME}\" as the\"StartUp Project\".")
endif()
