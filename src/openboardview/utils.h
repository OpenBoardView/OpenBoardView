#pragma once

#include <string>
#include <vector>

// Loads an entire file in to memory
std::vector<char> file_as_buffer(const std::string &utf8_filename);

// Extract extension from filename and check against given fileext
// fileext must be lowercase
bool check_fileext(const std::string &filename, const std::string fileext);

// Retunrs true if the given str was found in buf
bool find_str_in_buf(const std::string str, const std::vector<char> &buf);
