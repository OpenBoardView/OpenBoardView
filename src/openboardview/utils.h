#pragma once

#include <string>
#include <vector>

#include <SDL.h>

#include "filesystem_impl.h"

// Verify predicate X, if false write error to ERROR_MSG and log and execute ACTION
#define ENSURE_OR_FAIL(X, ERROR_MSG, ACTION) if (!(X)) { \
		ERROR_MSG = std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " + __PRETTY_FUNCTION__ + ": Assertion `" + #X + "' failed."; \
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", ERROR_MSG.c_str()); \
		ACTION; \
	}
// Same but no ACTION
#define ENSURE(X, ERROR_MSG) ENSURE_OR_FAIL(X, ERROR_MSG, )

// Loads an entire file in to memory
std::vector<char> file_as_buffer(const filesystem::path &filepath, std::string &error_msg);

// Extract extension from filename and check against given fileext
// fileext must be lowercase
bool check_fileext(const filesystem::path &filepath, const std::string fileext);

// Retunrs true if the given str was found in buf
bool find_str_in_buf(const std::string str, const std::vector<char> &buf);

// Case insensitive comparison of std::string
bool compare_string_insensitive(const std::string &str1, const std::string &str2);

// Case insensitive lookup of a filename at the given path
filesystem::path lookup_file_insensitive(const filesystem::path &path, const std::string &filename, std::string &error_msg);

// Split a string in a vector, delimiter is a space (stringstream iterator)
std::vector<std::string> split_string(const std::string str);

// Split a string in a vector with given delimiter
std::vector<std::string> split_string(const std::string &str, char delimeter);
