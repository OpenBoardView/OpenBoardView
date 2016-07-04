### asset/CMakeLists.txt ###
#
#

if (MSVC)
	# Needed for MSVS resource compiler
	set(ICON "${CMAKE_CURRENT_SOURCE_DIR}/OpenBoardView.rc" PARENT_SCOPE)
elseif(APPLE)
	# Used for app bundle
	set(ICON "${CMAKE_CURRENT_SOURCE_DIR}/brd.icns" PARENT_SCOPE)
	set(PLIST "${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in" PARENT_SCOPE)
else()
	# Assume a FreeDesktop.org system, e.g. linux
	configure_file("desktop.in" "${PROJECT_NAME}.desktop")
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.desktop"
	DESTINATION "${INSTALL_SHARE_DIR}/applications/")

	# MimeTypes, should be refreshed with update-mime-database /usr/share/mime
	install(FILES "mime.xml"
		DESTINATION "${INSTALL_SHARE_DIR}/mime/packages"
		RENAME "${PROJECT_NAME}.xml")

	# Icons
	# Add icons to the hicolor theme (used by at lease GNOME and KDE)
	install(DIRECTORY "hicolor"
		DESTINATION "${INSTALL_SHARE_DIR}/icons/"
		PATTERN "README*" EXCLUDE)
	# Copy the 48px icon to pixmaps too (extra compatibility)
	install(FILES "hicolor/48x48/apps/${PROJECT_NAME}.png"
		DESTINATION "${INSTALL_SHARE_DIR}/pixmaps/")

	# [Un]Install scripts
	# Currently only used to update MimeTypes
	configure_file("deb_install.sh.in" "postinst" @ONLY)
	configure_file("deb_install.sh.in" "postrm"   @ONLY)
	# Deb
	set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
	"${CMAKE_CURRENT_BINARY_DIR}/postinst"
	"${CMAKE_CURRENT_BINARY_DIR}/postrm")
	# RPM
	set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/postinst")
	set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/postrm")
endif()

# WiX Installer images
set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}\\\\brd.ico" PARENT_SCOPE) # icon for Add/Remove with WiX installer
set(CPACK_WIX_UI_BANNER "${CMAKE_CURRENT_SOURCE_DIR}\\\\wix_installer_banner.bmp" PARENT_SCOPE) # 493x58px bmp top banner for WiX installer
set(CPACK_WIX_UI_DIALOG "${CMAKE_CURRENT_SOURCE_DIR}\\\\wix_installer_bg.bmp" PARENT_SCOPE) # 493x312 bmp background image for WiX Welcome/Completion page

# Installer text resources
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE.txt" PARENT_SCOPE) # License
set(CPACK_RESOURCE_FILE_README  "${PROJECT_SOURCE_DIR}/README.md" PARENT_SCOPE) # License
set(CPACK_RESOURCE_FILE_WELCOME "${CMAKE_CURRENT_SOURCE_DIR}/Installer_Welcome.txt" PARENT_SCOPE) # Welcome text
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Package_Description.txt") #
