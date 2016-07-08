### CMakeLists.txt ###
#
# Run by CMake to generate project solution or makefiles
#

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
cmake_policy(SET CMP0048 NEW) # Use new style: version set by project()

# For when we set property CXX_STANDARD.
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable solution folders (in IDEs)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Name the solution folder for targets like INSTALL, PACKAGE, ZERO_CHECK
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "Meta Targets")

## MetaData
include(Project.cmake)

## Add library targets and create ${LIBS} and ${PLATFORM_LIBS}
add_subdirectory(lib)

# Add libs to a solution folder
set_target_properties(${LIBS} PROPERTIES FOLDER Libraries)

## Add assets
add_subdirectory(asset)

## Add main project
add_subdirectory(src)

# New feature in CMake 3.6 lets us define the Visual Studio StartUp Project
set_property(
	DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})


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
		"\"${PROJECT_NAME}\" as the \"StartUp Project\".")
endif()

## Keep track of whether cmake has been configured yet
set(CONFIGURED ON CACHE INTERNAL "" FORCE)
