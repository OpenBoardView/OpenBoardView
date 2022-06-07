# Copyright (C) 2018, 2022 Maxim Cournoyer
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the LICENSE file distributed with OpenBoardView.
#	Note:
#		Searching headers and libraries is very simple and is NOT as powerful as scripts
#		distributed with CMake, because LuaDist defines directories to search for.
#		Everyone is encouraged to contact the author with improvements. Maybe this file
#		becomes part of CMake distribution sometimes.

# - Find ImGui
# Find the native imgui headers and libraries.
#
# IMGUI_INCLUDE_DIRS	- where to find imgui/imgui.h, etc.
# IMGUI_LIBRARIES	- List of libraries when using imgui.
# IMGUI_FOUND	        - True if imgui is found.

# Look for the header file.
FIND_PATH(IMGUI_INCLUDE_DIR NAMES imgui.h PATH_SUFFIXES imgui)

# Look for the library.
FIND_LIBRARY(IMGUI_LIBRARY NAMES ImGui imgui)

# Handle the QUIETLY and REQUIRED arguments and set IMGUI_FOUND to TRUE if all listed variables are TRUE.
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ImGui DEFAULT_MSG IMGUI_LIBRARY IMGUI_INCLUDE_DIR)

# Copy the results to the output variables.
IF(IMGUI_FOUND)
	SET(IMGUI_LIBRARIES ${IMGUI_LIBRARY})
	SET(IMGUI_INCLUDE_DIRS ${IMGUI_INCLUDE_DIR})
ELSE()
	SET(IMGUI_LIBRARIES)
	SET(IMGUI_INCLUDE_DIRS)
ENDIF()

MARK_AS_ADVANCED(IMGUI_INCLUDE_DIRS IMGUI_LIBRARIES)
