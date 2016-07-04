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

	imgui_impl_dx9.cpp
	platform.cpp)

## Add main target
add_executable(${PROJECT_NAME}
	${SOURCES}
	${ASSETS})

target_link_libraries(${PROJECT_NAME}
	imgui
	 ${PLATFORM_LIBS}
	)

set_target_properties(${PROJECT_NAME} PROPERTIES
		CXX_STANDARD 11       # Use C++11
		WIN32_EXECUTABLE TRUE # Set OpenBoardView as a (WinMain) GUI app on Windows
)

install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR})
