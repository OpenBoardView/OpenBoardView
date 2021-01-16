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


#ifdef _MSC_VER
#include <io.h>
#ifndef F_OK
#define F_OK 0
#endif
#endif // _MSC_VER

// Filesystem fallback to ghc::filesystem if std::filesystem not available
#if __cplusplus >= 201703L && defined(__has_include)
#if __has_include(<filesystem>)
#define GHC_USE_STD_FS
#include <filesystem>
namespace filesystem = std::filesystem;
#endif
#endif
#ifndef GHC_USE_STD_FS
#include <ghc/filesystem.hpp>
namespace filesystem = ghc::filesystem;
#endif

// Shows a file dialog (should hang the current thread) and returns the utf8
// filename picked by the user.
const std::string show_file_picker(bool filterBoards = false);

const std::string get_font_path(const std::string &name);
const std::vector<char> load_font(const std::string &name);

enum class UserDir { Config, Data };
const std::string get_user_dir(const UserDir userdir);

#ifdef _WIN32
char *strcasestr(const char *str, const char *pattern);
#endif // _WIN32

// File stat queries, cross platform.
#include <sys/stat.h>

#ifdef _WIN32
#if defined(_MSC_VER) || defined(__MINGW64__)
typedef struct _stat64 path_stat_t;
#elif defined(__MINGW32__)
typedef struct _stati64 path_stat_t;
#else
typedef struct _stat path_stat_t;
#endif
#ifndef S_ISDIR
#define S_ISDIR(x) (((x) & _S_IFDIR) == _S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(x) (((x) & _S_IFREG) == _S_IFREG)
#endif
#else
typedef struct stat path_stat_t;
#endif

int path_stat(const std::string& path, path_stat_t *st);
