set(ASSETS)

if(APPLE)
	configure_file("icon.icns" "${PROJECT_NAME_LOWER}.icns" COPYONLY)

	SET(ASSETS ${ASSETS}
	    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME_LOWER}.icns")

elseif(WIN32)
	configure_file("project.rc" "${PROJECT_NAME}.rc" @ONLY)
	configure_file("icon.ico" "${PROJECT_NAME}.ico" COPYONLY)

	SET(ASSETS ${ASSETS}
	    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rc"
	    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.ico")

elseif(UNIX)
	configure_file("app.desktop" "${PROJECT_NAME_LOWER}.desktop" @ONLY)
	configure_file("appdata.xml" "${PROJECT_NAME_LOWER}.appdata.xml" @ONLY)

	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME_LOWER}.desktop"
	        DESTINATION "share/applications")
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME_LOWER}.appdata.xml"
	        DESTINATION "share/appdata")

	install(FILES "icon.svg"
	        DESTINATION "share/icons/hicolor/scalable/apps"
	        RENAME "${PROJECT_NAME_LOWER}.svg")
	install(FILES "mimetypes.xml"
	        DESTINATION "share/mime/packages"
	        RENAME "${PROJECT_NAME_LOWER}.xml")

	set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
	    "${CMAKE_CURRENT_SOURCE_DIR}/postinst"
	    "${CMAKE_CURRENT_SOURCE_DIR}/postrm"
	    PARENT_SCOPE)
	set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE
	    "${CMAKE_CURRENT_SOURCE_DIR}/postinst"
	    PARENT_SCOPE)
	set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE
	    "${CMAKE_CURRENT_SOURCE_DIR}/postrm"
	    PARENT_SCOPE)

endif()

SET(ASSETS ${ASSETS} PARENT_SCOPE)
