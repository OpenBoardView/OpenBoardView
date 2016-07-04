
## Add CMake Modules (FindGLFW3.cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/CMake")

### System libraries
if(WIN32)
	set(PLATFORM_LIBS ${PLATFORM_LIBS}
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
elseif(APPLE)
	# Pass -framework flag to linker and also link objc for platform_osx.mm
	set(PLATFORM_LIBS ${PLATFORM_LIBS} "-framework Cocoa" objc)
else()
	# On other systems, just use GTK+ 3
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

	set(INCLUDE_DIRS ${INCLUDE_DIRS} ${GTK3_INCLUDE_DIRS})
	set(PLATFORM_LIBS ${PLATFORM_LIBS} ${GTK3_LIBRARIES})
endif()

## OpenGL
find_package(OpenGL REQUIRED)

if(NOT OPENGL_FOUND)
	message(FATAL_ERROR "OpenGL not found!") # TODO: instructions on how to fix using cmake-gui
else()
	set(PLATFORM_LIBS ${PLATFORM_LIBS} ${OPENGL_LIBRARIES})
endif()

## glfw ##
option(USE_SYSTEM_GLFW
	"Try to use the system installed GLFW. Will fallback to included version if GLFW can't be found."
ON)
if(USE_SYSTEM_GLFW)
	find_package(GLFW3 3.2)
	if(GLFW3_FOUND)
		message("\n  Using system GLFW\n")
		set(PLATFORM_LIBS ${PLATFORM_LIBS} ${GLFW3_LIBRARY})
		# Link_dirs? Include_dirs?
	endif()
endif()
