### src/CMakeLists.txt ###
#
#

# SOURCES to be added to main target
set(SOURCES
	main.cpp

	BoardView.cpp
	BRDBoard.cpp
	BRDFile.cpp
	BDVFile.cpp
	NetList.cpp
	PartList.cpp

	imgui_impl_dx9.cpp
	platform_win32.cpp)

## Add main target
add_executable(${PROJECT_NAME}
	WIN32
	${SOURCES}
	${ASSETS})

target_link_libraries(${PROJECT_NAME} PRIVATE
	${LIBS}
	${PLATFORM_LIBS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${INCLUDE_DIRS}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
		CXX_STANDARD 11       # Use C++11
)

if (MSVC)
	target_compile_definitions(${PROJECT_NAME} PRIVATE
		-D_CRT_SECURE_NO_WARNINGS
		-D_UNICODE -DUNICODE)
endif()

# Warnings
set(SHOW_WARNINGS "normal" CACHE STRING "Compile time warnings to show. Can be \"normal\", \"all\" or \"all_extra\".")
set_property(CACHE SHOW_WARNINGS PROPERTY STRINGS "normal" "all" "all_extra")
option(WARNINGS_ARE_ERRORS "Compile time warnings are treated as errors."
	OFF)
if(SHOW_WARNINGS STREQUAL "all")
	if(MSVC)
		set(WARN_FLAGS "-W4")
	endif()
elseif(SHOW_WARNINGS STREQUAL "all_extra")
	if(MSVC)
		set(WARN_FLAGS "-Wall")
	endif()
endif()
if(WARNINGS_ARE_ERRORS)
	if(MSVC)
		set(WARN_FLAGS ${WARN_FLAGS} "-WX")
	endif()
endif()
if(WARN_FLAGS)
	target_compile_options(${PROJECT_NAME} PRIVATE ${WARN_FLAGS})
endif()

# Inastll/packageing location
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${INSTALL_BIN_DIR})
