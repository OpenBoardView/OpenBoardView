set(ASSETS
)

if(APPLE)
	SET(ASSETS ${ASSETS}
		${CMAKE_CURRENT_SOURCE_DIR}/brd.icns
	)
elseif(UNIX)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ofbv-app.svg
		DESTINATION share/icons/hicolor/scalable/apps RENAME openboardview.svg)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/openboardview.appdata.xml
		DESTINATION share/appdata)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/openboardview.desktop
		DESTINATION share/applications)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/openboardview.xml
		DESTINATION share/mime/packages)
elseif(WIN32)
	SET(ASSETS ${ASSETS}
		${CMAKE_CURRENT_SOURCE_DIR}/OpenBoardView.rc
	)
endif()

SET(ASSETS ${ASSETS} PARENT_SCOPE)
