#pragma once

#include <string>
#include <vector>

#include <SDL.h>

#include "filesystem_impl.h"

#define ENSURE(X) if (!(X)) SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s:%d: %s: Assertion `%s' failed.", __FILE__, __LINE__, __PRETTY_FUNCTION__, #X);

// Loads an entire file in to memory
std::vector<char> file_as_buffer(const filesystem::path &filepath);

// Extract extension from filename and check against given fileext
// fileext must be lowercase
bool check_fileext(const filesystem::path &filepath, const std::string fileext);

// Retunrs true if the given str was found in buf
bool find_str_in_buf(const std::string str, const std::vector<char> &buf);

// Case insensitive comparison of std::string
bool compare_string_insensitive(const std::string &str1, const std::string &str2);

// Case insensitive lookup of a filename at the given path
filesystem::path lookup_file_insensitive(const filesystem::path &path, const std::string &filename);

// Split a string in a vector, delimiter is a space (stringstream iterator)
std::vector<std::string> split_string(const std::string str);

// Split a string in a vector with given delimiter
std::vector<std::string> split_string(const std::string &str, char delimeter);
