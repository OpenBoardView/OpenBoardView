option(ENABLE_GL1 "Build OpenGL 1 renderer." ON)
option(ENABLE_GL3 "Build OpenGL 3 renderer." ON)
option(ENABLE_GLES2 "Build OpenGL ES 2 renderer." ON)

if(ENABLE_GL1)
	add_definitions(-DENABLE_GL1)
endif()
if(ENABLE_GL3)
	add_definitions(-DENABLE_GL3)
endif()
if(ENABLE_GLES2)
	add_definitions(-DENABLE_GLES2)
endif()

# Platform-specific configuration
if(WIN32)
	add_definitions(-DUNICODE)
	add_definitions(-D_UNICODE)
else()
if(APPLE)
        find_library(COCOA_LIBRARY Cocoa)
	find_package(OpenGL REQUIRED)
	set(OPENGL_INCLUDE_DIR ${OPENGL_INCLUDE_DIR}/Headers)
	else(APPLE)
find_package(PkgConfig REQUIRED)

		pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
		if(FONTCONFIG_FOUND)
			message(STATUS "Found FontConfig version ${FONTCONFIG_VERSION}")
			link_directories(${FONTCONFIG_LIBRARY_DIRS})
			add_definitions(${FONTCONFIG_CFLAGS_OTHER})
			add_definitions(-DENABLE_FONTCONFIG)
		endif()

		pkg_search_module(GTK REQUIRED gtk+-3.0 gtk+-2.0) # gtk2 fallback if gtk3 not found
		if(GTK_FOUND)
			message(STATUS "Found GTK version ${GTK_VERSION}")
			link_directories(${GTK_LIBRARY_DIRS}) # not linked since we load it at runtime
			add_definitions(${GTK_CFLAGS_OTHER})
			add_definitions(-DENABLE_GTK)
		else()
			message(WARNING "GTK not found, file picker will be unavailable.")
		endif()
	endif(APPLE)
endif()

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_SOURCE_DIR}/../imgui
	${SDL2_INCLUDE_DIRS}
	${GLAD_INCLUDE_DIRS}
	${GTK_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIRS}
	${SQLITE3_INCLUDE_DIRS}
	${FONTCONFIG_INCLUDE_DIRS}
)

link_directories(
	${SDL2_LIBRARY_DIRS}
)


set(SOURCES
	annotations.cpp
	confparse.cpp
	vectorhulls.cpp
	history.cpp
	utils.cpp
	BoardView.cpp
	BRDBoard.cpp
	FileFormats/ASCFile.cpp
	FileFormats/BDVFile.cpp
	FileFormats/BRD2File.cpp
	FileFormats/BRDFile.cpp
	FileFormats/BVRFile.cpp
	FileFormats/CSTFile.cpp
	FileFormats/FZFile.cpp
	NetList.cpp
	PartList.cpp
	Searcher.cpp
	SpellCorrector.cpp
	main_opengl.cpp
)

if(ENABLE_GL1)
	LIST(APPEND SOURCES
		Renderers/imgui_impl_sdl.cpp
	)
endif()
if(ENABLE_GL3)
	LIST(APPEND SOURCES
		Renderers/imgui_impl_sdl_gl3.cpp
	)
endif()
if(ENABLE_GLES2)
	LIST(APPEND SOURCES
		Renderers/imgui_impl_sdl_gles2.cpp
	)
endif()

if(WIN32)
	set(SOURCES ${SOURCES}
		win32.cpp
	)
else()
if(APPLE)
	set(SOURCES ${SOURCES}
		osx.mm
	)
endif()
	set(SOURCES ${SOURCES}
		unix.cpp
	)
endif()

# Must be defined in the same directory as the add_executable including the file
set_source_files_properties(${ASSETS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

add_executable(${PROJECT_NAME_LOWER}
	MACOSX_BUNDLE
	WIN32
	${SOURCES}
	${ASSETS}
)

if(MINGW) # Dirty fix to force linking all libs (esp. libstdc++) statically for Windows
	set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
	set_target_properties(${PROJECT_NAME_LOWER} PROPERTIES LINK_SEARCH_END_STATIC 1)
elseif(APPLE)
	set_target_properties(${PROJECT_NAME_LOWER} PROPERTIES MACOSX_BUNDLE_ICON_FILE ${PROJECT_NAME_LOWER})
endif()

target_link_libraries(${PROJECT_NAME_LOWER}
	imgui
	${GLAD_LIBRARIES}
	${COCOA_LIBRARY}
	${ZLIB_LIBRARIES}
	${SQLITE3_LIBRARIES}
)

if(MINGW) # Link statically with SDL2 for Windows
	target_link_libraries(${PROJECT_NAME_LOWER}
		${SDL2_STATIC_LIBRARIES}
	)
else()
	if(NOT APPLE)
		target_link_libraries(${PROJECT_NAME_LOWER}
			${FONTCONFIG_LIBRARIES}
		)
	endif()
	target_link_libraries(${PROJECT_NAME_LOWER}
		${SDL2_LIBRARIES}
	)
endif()
if(NOT WIN32)
	target_link_libraries(${PROJECT_NAME_LOWER}
		dl
	)
endif()

install(TARGETS
	${PROJECT_NAME_LOWER}
	RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR}
	BUNDLE DESTINATION ${INSTALL_BUNDLE_DIR})
