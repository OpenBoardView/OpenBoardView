#pragma once

#ifdef _WIN32
#ifdef _MSC_VER // Visual Studio prefixes non-standard C functions with _
#define _CRT_SECURE_NO_WARNINGS 1
#endif                                  // _MSC_VER
#define NTDDI_VERSION NTDDI_VISTA       // Not caring about XP. If this is a concern, find a replacement for SHGetKnownFolderPath()
#define _WIN32_WINNT _WIN32_WINNT_VISTA // and CompareStringEx() in win32.cpp.
#include <windows.h>
#endif // _WIN32

#include <string>
#include <vector>

#include "filesystem_impl.h"

#ifdef _MSC_VER
#include <io.h>
#ifndef F_OK
#define F_OK 0
#endif
#endif // _MSC_VER

// Shows a file dialog (should hang the current thread) and returns the utf8
// filename picked by the user.
const filesystem::path show_file_picker(bool filterBoards = false);

const std::string get_font_path(const std::string &name);
const std::vector<char> load_font(const std::string &name);

enum class UserDir { Config, Data };
const std::string get_user_dir(const UserDir userdir);

#ifdef _WIN32
char *strcasestr(const char *str, const char *pattern);
#endif // _WIN32
