find_package(SDL2 REQUIRED) # is this really required by windows?

# Link with Cocoa on OS X and GTK3 on other Unix (or MinGW)
if(UNIX)
if(APPLE)
        find_library(COCOA_LIBRARY Cocoa)
	find_package(OpenGL REQUIRED)
	set(OPENGL_INCLUDE_DIR ${OPENGL_INCLUDE_DIR}/Headers)
	else(APPLE)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})
	endif(APPLE)
endif(UNIX)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_SOURCE_DIR}/../imgui
	${CMAKE_CURRENT_SOURCE_DIR}/../gl3w/include
	${SDL2_INCLUDE_DIR}
	${GTK3_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR}
)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
if(MINGW)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUNICODE -D_UNICODE")
endif()


set(SOURCES
	BoardView.cpp
	BRDBoard.cpp
	BRDFile.cpp
	imgui_impl_sdl_gl3.cpp
	main_opengl.cpp
	NetList.cpp
	PartList.cpp
	TextureDDS.cpp
)

if(UNIX)
if(APPLE)
	set(SOURCES ${SOURCES}
		osx.mm
	)
endif()
	set(SOURCES ${SOURCES}
		unix.cpp
	)
elseif (MINGW) # Should this just be WIN32?
	set(SOURCES ${SOURCES}
		win32.cpp
	)
endif()



add_executable(openboardview
#	MACOSX_BUNDLE
	${SOURCES}
	${ASSETS}
)

target_link_libraries(openboardview
	dl
	imgui
	gl3w
	${COCOA_LIBRARY}
	${SDL2_LIBRARY}
	${GTK3_LIBRARIES}
)

install(TARGETS
	openboardview
	RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR}
	BUNDLE DESTINATION ${INSTALL_BUNDLE_DIR})

if (MINGW)
	install(DIRECTORY ${PROJECT_SOURCE_DIR}/${INSTALL_ASSET_DIR} DESTINATION ${INSTALL_RUNTIME_DIR})
endif ()


add_custom_target(install_obv
	${CMAKE_MAKE_PROGRAM} install
	DEPENDS openboardview
	COMMENT "Installing ${PROJECT_NAME}")
