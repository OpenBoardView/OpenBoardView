// Filesystem fallback to ghc::filesystem if std::filesystem not available
#if __cplusplus >= 201703L && defined(__has_include)
#if __has_include(<filesystem>)
#define GHC_USE_STD_FS
#include <filesystem>
namespace filesystem = std::filesystem;
using ifstream = std::ifstream;
using ofstream = std::ofstream;
using fstream = std::fstream;
#endif
#endif
#ifndef GHC_USE_STD_FS
#define GHC_WIN_WSTRING_STRING_TYPE
#include <ghc/filesystem.hpp>
namespace filesystem = ghc::filesystem;
using ifstream = ghc::filesystem::ifstream;
using ofstream = ghc::filesystem::ofstream;
using fstream = ghc::filesystem::fstream;
#endif
