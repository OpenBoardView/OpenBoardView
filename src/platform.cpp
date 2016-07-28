#include "platform.h"
#include "BoardView.h"

#include <assert.h>
#include <nowide/fstream.hpp>
#include <string>

char *file_as_buffer(size_t *buffer_size, const char *utf8_filename) {
	return file_as_buffer(buffer_size, std::string(utf8_filename));
}
char *file_as_buffer(size_t *buffer_size, std::string &utf8_filename) {
	nowide::ifstream file;
	file.open(utf8_filename, std::ios::in | std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		std::string error = "Error opening \"" + utf8_filename + "\": " + strerror(errno);
		app.ShowError(error);
		*buffer_size = 0;
		return nullptr;
	}

	std::streampos sz = file.tellg();
	*buffer_size = sz;
	char *buf = (char *)malloc(sz);
	file.seekg(0, std::ios::beg);
	file.read(buf, sz);
	assert(file.gcount() == sz);
	file.close();

	return buf;
}
