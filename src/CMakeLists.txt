### src/CMakeLists.txt ###
#
#

# SOURCES to be added to main target
set(SOURCES
	main.cpp

	BoardView.cpp
	BRDBoard.cpp
	BRDFile.cpp
	NetList.cpp
	PartList.cpp

	imgui_impl_glfw.cpp
)

# Add platform specific sources
if(WIN32)
	SET(SOURCES ${SOURCES}
		platform_win32.cpp
	)
elseif(APPLE)
	SET(SOURCES ${SOURCES}
		platform_osx.mm
		platform_unix.cpp
	)
elseif(UNIX AND NOT APPLE)
	SET(SOURCES ${SOURCES}
		platform_unix.cpp
	)
endif()

## Add main target
add_executable(${PROJECT_NAME}
	WIN32 MACOSX_BUNDLE
	${SOURCES}
	${ICON})

target_link_libraries(${PROJECT_NAME} PRIVATE
	${LIBS}
	${PLATFORM_LIBS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${INCLUDE_DIRS}
)

target_compile_options(${PROJECT_NAME} PRIVATE ${CFLAGS})

set_target_properties(${PROJECT_NAME} PROPERTIES
		CXX_STANDARD 11       # Use C++11
)

if (MSVC)
	target_compile_definitions(${PROJECT_NAME} PRIVATE
		-D_CRT_SECURE_NO_WARNINGS
		-D_UNICODE -DUNICODE)

	# Tell MSVC to use main instead of WinMain for Windows subsystem executables
	set_target_properties(${PROJECT_NAME} PROPERTIES
		LINK_FLAGS "/subsystem:windows /ENTRY:mainCRTStartup")
elseif(APPLE)
	# Help CPack build the bundle
	set_source_files_properties(${ICON} PROPERTIES
		MACOSX_PACKAGE_LOCATION "Resources")

	set_target_properties(${PROJECT_NAME} PROPERTIES
		MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
		MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
		MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION}
		MACOSX_BUNDLE_ICON_FILE "brd"
		MACOSX_BUNDLE_INFO_PLIST ${PLIST})
endif()

# Warnings
set(SHOW_WARNINGS "normal" CACHE STRING "Compile time warnings to show. Can be \"normal\", \"all\" or \"all_extra\".")
set_property(CACHE SHOW_WARNINGS PROPERTY STRINGS "normal" "all" "all_extra")
option(WARNINGS_ARE_ERRORS "Compile time warnings are treated as errors."
	OFF)
if(SHOW_WARNINGS STREQUAL "all")
	if(MSVC)
		set(WARN_FLAGS "-W4")
	else()
		set(WARN_FLAGS "-Wall")
	endif()
elseif(SHOW_WARNINGS STREQUAL "all_extra")
	if(MSVC)
		set(WARN_FLAGS "-Wall")
	else()
		set(WARN_FLAGS "-Wall -Wextra")
	endif()
endif()
if(WARNINGS_ARE_ERRORS)
	if(MSVC)
		set(WARN_FLAGS ${WARN_FLAGS} "-WX")
	else()
		set(WARN_FLAGS ${WARN_FLAGS} "-Werror")
	endif()
endif()
if(WARN_FLAGS)
	target_compile_options(${PROJECT_NAME} PRIVATE ${WARN_FLAGS})
endif()

# Inastll/packageing location
install(TARGETS ${PROJECT_NAME}
	BUNDLE DESTINATION .
	RUNTIME DESTINATION ${INSTALL_BIN_DIR})
