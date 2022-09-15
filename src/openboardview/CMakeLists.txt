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
			add_definitions(${FONTCONFIG_CFLAGS} ${FONTCONFIG_CFLAGS_OTHER})
			add_definitions(-DENABLE_FONTCONFIG)
		endif()

		pkg_search_module(GTK gtk+-3.0 gtk+-2.0) # gtk2 fallback if gtk3 not found
		if(GTK_FOUND)
			message(STATUS "Found GTK version ${GTK_VERSION}")
			link_directories(${GTK_LIBRARY_DIRS}) # not linked since we load it at runtime
			add_definitions(${GTK_CFLAGS} ${GTK_CFLAGS_OTHER})
			add_definitions(-DENABLE_GTK)
		else()
			message(WARNING "GTK not found, file picker will be unavailable.")
		endif()

		pkg_search_module(GIO REQUIRED gio-2.0)
		if(GIO_FOUND)
			message(STATUS "Found GIO version ${GIO_VERSION}")
			link_directories(${GIO_LIBRARY_DIRS})
			add_definitions(${GIO_CFLAGS} ${GIO_CFLAGS_OTHER})
			add_definitions(-DENABLE_PDFBRIDGE_EVINCE)
		else()
			message(SEND_ERROR "GIO not found, PDFBridgeEvince cannot be enabled.")
		endif()
	endif(APPLE)
endif()

# python is required for GenCAD grammar build-rime generation
if (CMAKE_VERSION VERSION_GREATER 3.12)
	find_package(Python REQUIRED COMPONENTS Interpreter)
else()
	# deprecated logic
	find_package(PythonInterp REQUIRED)
	set(Python_EXECUTABLE "${PYTHON_EXECUTABLE}")
endif()

set(SOURCES
	annotations.cpp
	confparse.cpp
	vectorhulls.cpp
	history.cpp
	utils.cpp
	BoardView.cpp
	Board.cpp
	BRDBoard.cpp
	FileFormats/BRDFileBase.cpp
	FileFormats/ADFile.cpp
	FileFormats/ASCFile.cpp
	FileFormats/BDVFile.cpp
	FileFormats/BRD2File.cpp
	FileFormats/BRDFile.cpp
	FileFormats/BVRFile.cpp
	FileFormats/BVR3File.cpp
	FileFormats/CADFile.cpp
	FileFormats/CSTFile.cpp
	FileFormats/FZFile.cpp
	FileFormats/GenCADFile.cpp
	NetList.cpp
	PartList.cpp
	Renderers/Renderers.cpp
	Renderers/ImGuiRendererSDL.cpp
	Searcher.cpp
	SpellCorrector.cpp
	UI/Keyboard/KeyBinding.cpp
	UI/Keyboard/KeyBindings.cpp
	UI/Keyboard/KeyModifiers.cpp
	GUI/BackgroundImage.cpp
	GUI/Image.cpp
	GUI/Preferences/BoardSettings/BackgroundImage.cpp
	GUI/Preferences/BoardSettings/BoardSettings.cpp
	GUI/Preferences/BoardSettings/PDFFile.cpp
	GUI/Preferences/Keyboard.cpp
	PDFBridge/PDFBridge.cpp
	PDFBridge/PDFFile.cpp
	main_opengl.cpp
)

if(ENABLE_GL1)
	LIST(APPEND SOURCES
		Renderers/ImGuiRendererSDLGL1.cpp
	)
endif()
if(ENABLE_GL3)
	LIST(APPEND SOURCES
		Renderers/ImGuiRendererSDLGL3.cpp
	)
endif()

if(GIO_FOUND)
	set(SOURCES ${SOURCES}
		PDFBridge/PDFBridgeEvince.cpp
	)
endif()

if(WIN32)
	set(SOURCES ${SOURCES}
		PDFBridge/PDFBridgeSumatra.cpp
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

set(GENCAD_FILE_GRAMMAR_GENERATOR
	"${CMAKE_CURRENT_SOURCE_DIR}/../../utilities/generate_grammar_header.py")

set(GENCAD_FILE_BNF_H
	"${CMAKE_CURRENT_SOURCE_DIR}/FileFormats/GenCADFileBnf.h")
set(GENERATED_GENCAD_FILE_GRAMMAR_H
	"${CMAKE_CURRENT_BINARY_DIR}/build-generated/GenCADFileGrammar.h")

add_custom_command(OUTPUT "${GENERATED_GENCAD_FILE_GRAMMAR_H}"
	COMMAND "${Python_EXECUTABLE}" "${GENCAD_FILE_GRAMMAR_GENERATOR}" "${GENCAD_FILE_BNF_H}" "${GENERATED_GENCAD_FILE_GRAMMAR_H}"
	DEPENDS "${GENCAD_FILE_GRAMMAR_GENERATOR}" "${GENCAD_FILE_BNF_H}"
)

set(SOURCES ${SOURCES}
	${GENERATED_GENCAD_FILE_GRAMMAR_H} # dependency on generated header file to make in built
)

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

target_include_directories(${PROJECT_NAME_LOWER} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_BINARY_DIR} # for build-generated
	${IMGUI_INCLUDE_DIRS}
	${UTF8_INCLUDE_DIR}
	${GLAD_INCLUDE_DIRS}
	${GTK_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIRS}
	${FONTCONFIG_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME_LOWER}
	imgui
	SQLite::SQLite3
	mpc
	${GLAD_LIBRARIES}
	${COCOA_LIBRARY}
	${ZLIB_LIBRARIES}
	${FILESYSTEM_LIBRARIES}
	${CMAKE_DL_LIBS}
)

if(NOT APPLE AND NOT MINGW)
	target_link_libraries(${PROJECT_NAME_LOWER}
		${FONTCONFIG_LIBRARIES}
	)
endif()

if(GIO_FOUND)
	target_link_libraries(${PROJECT_NAME_LOWER}
		${GIO_LIBRARIES}
	)
endif()


if(MINGW)
target_link_libraries(${PROJECT_NAME_LOWER}
	SDL2::SDL2-static
)
else()
target_link_libraries(${PROJECT_NAME_LOWER}
	SDL2::SDL2
)
endif()
target_link_libraries(${PROJECT_NAME_LOWER}
	SDL2::SDL2main
)

install(TARGETS
	${PROJECT_NAME_LOWER}
	RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR}
	BUNDLE DESTINATION ${INSTALL_BUNDLE_DIR})
