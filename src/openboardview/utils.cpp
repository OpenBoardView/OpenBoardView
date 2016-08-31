#include "platform.h"
#include "utils.h"
#include <algorithm>
#include <assert.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdint.h>

#include <sys/stat.h>

// Loads an entire file in to memory
std::vector<char> file_as_buffer(const std::string &utf8_filename) {
	std::vector<char> data;
	std::ifstream file;
	file.open(utf8_filename, std::ios::in | std::ios::binary | std::ios::ate);

	if (!file.is_open()) {
		std::cerr << "Error opening " << utf8_filename << ": " << strerror(errno) << std::endl;
		return data;
	}

	file.seekg(0, std::ios_base::end);
	std::streampos sz = file.tellg();
	assert(sz >= 0);
	data.reserve(sz);
	file.seekg(0, std::ios_base::beg);
	data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

	assert(data.size() == static_cast<unsigned int>(sz));
	file.close();

	return data;
}

// Extract extension from filename and check against given fileext
// fileext must be lowercase
bool check_fileext(const std::string &filename, const std::string fileext) {
	auto extpos     = filename.rfind('.');
	std::string ext = (extpos == std::string::npos) ? "" : filename.substr(extpos); // extract file ext
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);                 // make ext lowercase
	return ext == fileext;
}

// Retunrs true if the given str was found in buf
bool find_str_in_buf(const std::string str, const std::vector<char> &buf) {
	return std::search(buf.begin(), buf.end(), str.begin(), str.end()) != buf.end();
}
