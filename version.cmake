# Define project variables

project(OpenBoardView)
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
set(PROJECT_VERSION "R7.4")
set(PROJECT_URL "http://openboardview.org")
set(PROJECT_LICENSE "MIT")
file(READ "LICENSE" PROJECT_LICENSE_TEXT)

# Escape newlines so we don't break string literal syntax
string(REPLACE "\n" "\\n" PROJECT_LICENSE_TEXT_ESCAPED ${PROJECT_LICENSE_TEXT})
