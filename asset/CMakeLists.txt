set(ASSETS)

if(APPLE)
	configure_file("icon.icns" "${PROJECT_NAME_LOWER}.icns" COPYONLY)

	SET(ASSETS ${ASSETS}
	    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME_LOWER}.icns")

elseif(WIN32)
	configure_file("project.rc" "${PROJECT_NAME}.rc" COPYONLY)

	SET(ASSETS ${ASSETS}
	    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rc")

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
	    "postinst"
	    "postrm" PARENT_SCOPE)
	set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE
	    "postinst" PARENT_SCOPE)
	set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE
		"postrm" PARENT_SCOPE)

endif()

SET(ASSETS ${ASSETS} PARENT_SCOPE)
