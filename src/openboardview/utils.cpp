#include "utils.h"
#include <SDL.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <cstdint>

#ifndef _WIN32
#include <dirent.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>

// Loads an entire file in to memory
std::vector<char> file_as_buffer(const filesystem::path &filepath, std::string &error_msg) {
	std::vector<char> data;

	if (!filesystem::is_regular_file(filepath)) {
		error_msg = "Not a regular file";
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error opening %s: %s", filepath.string().c_str(), error_msg.c_str());
		return data;
	}

	ifstream file;
	file.open(filepath, std::ios::in | std::ios::binary | std::ios::ate);

	if (!file.is_open()) {
		error_msg = strerror(errno);
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error opening %s: %s", filepath.string().c_str(), error_msg.c_str());
		return data;
	}

	file.seekg(0, std::ios_base::end);
	std::streampos sz = file.tellg();
	ENSURE(sz >= 0, error_msg);
	data.reserve(sz);
	file.seekg(0, std::ios_base::beg);
	data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

	ENSURE(data.size() == static_cast<unsigned int>(sz), error_msg);
	file.close();

	return data;
}

// Extract extension from filename and check against given fileext
// fileext must be lowercase
bool check_fileext(const filesystem::path &filepath, const std::string fileext) {
	std::string ext{filepath.extension().string()}; // extract file ext
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);                 // make ext lowercase
	return ext == fileext;
}

// Returns true if the given str was found in buf
bool find_str_in_buf(const std::string str, const std::vector<char> &buf) {
	return std::search(buf.begin(), buf.end(), str.begin(), str.end()) != buf.end();
}

// Case insensitive comparison of std::string
bool compare_string_insensitive(const std::string &str1, const std::string &str2) {
	return str1.size() == str2.size() && std::equal(str2.begin(), str2.end(), str1.begin(), [](const char &a, const char &b) {
		       return std::tolower(a) == std::tolower(b);
		   });
}

// Case insensitive lookup of a filename at the given path
filesystem::path lookup_file_insensitive(const filesystem::path &path, const std::string &filename, std::string &error_msg) {
	std::error_code ec;
	filesystem::directory_iterator di{path, ec};

	if (ec) {
		error_msg = "Error looking up '" + filename + "' in '" + path.string().c_str() + "': " + ec.message();
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error looking up '%s' in '%s': %d - %s", filename.c_str(), path.string().c_str(), ec.value(), ec.message().c_str());
		return {};
	}

	for(auto& p: filesystem::directory_iterator(path)) {
		if (compare_string_insensitive(p.path().filename().string(), filename)) {
			return p.path();
		}
	}
	error_msg = filename + ": file not found in '" + path.string() + "'.";
	return {};
}

// Split a string in a vector, delimiter is a space (stringstream iterator)
std::vector<std::string> split_string(const std::string str) {
	std::istringstream iss(str);
	return {std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
}

// Split a string in a vector with given delimiter
std::vector<std::string> split_string(const std::string &str, char delimeter) {
	std::istringstream iss(str);
	std::string item;
	std::vector<std::string> strs;
	while (std::getline(iss, item, delimeter)) {
		strs.push_back(item);
	}
	return strs;
}
