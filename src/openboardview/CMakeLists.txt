find_package(SDL2 REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_SOURCE_DIR}/../imgui
	${CMAKE_CURRENT_SOURCE_DIR}/../json11
	${CMAKE_CURRENT_SOURCE_DIR}/../gl3w/include
	${SDL2_INCLUDE_DIR}
	${GTK3_INCLUDE_DIRS}
)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(openboardview
	BoardView.cpp
	BRDBoard.cpp
	BRDFile.cpp
	imgui_impl_sdl_gl3.cpp
	main_opengl.cpp
	NetList.cpp
	PartList.cpp
	TextureDDS.cpp
	unix.cpp
)

target_link_libraries(openboardview
	dl
	imgui
	gl3w
	json11
	${SDL2_LIBRARY}
	${GTK3_LIBRARIES}
)

install(TARGETS
	openboardview
	RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR})

