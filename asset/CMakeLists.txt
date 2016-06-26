### asset/CMakeLists.txt ###
#
#

# These files will be added to the main target later
set(ASSETS
  ${CMAKE_CURRENT_SOURCE_DIR}/OpenBoardView.rc # Needed for MSVS resource compiler
  ${CMAKE_CURRENT_SOURCE_DIR}/FiraSans-Medium.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/empty_circle.dds
  PARENT_SCOPE # Send this to everyone
)
