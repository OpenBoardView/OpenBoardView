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

	set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
		"${CMAKE_CURRENT_SOURCE_DIR}/postinst"
		"${CMAKE_CURRENT_SOURCE_DIR}/postrm" PARENT_SCOPE)
	set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE
		"${CMAKE_CURRENT_SOURCE_DIR}/postinst" PARENT_SCOPE)
	set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE
		"${CMAKE_CURRENT_SOURCE_DIR}/postrm" PARENT_SCOPE)
elseif(WIN32)
	SET(ASSETS ${ASSETS}
		${CMAKE_CURRENT_SOURCE_DIR}/OpenBoardView.rc
	)
endif()

SET(ASSETS ${ASSETS} PARENT_SCOPE)
