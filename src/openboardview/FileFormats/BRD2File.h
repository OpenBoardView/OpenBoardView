#pragma once

#include <cstring>

#include "BRDFile.h"
struct BRD2File : public BRDFile {
	BRD2File(std::vector<char> &buf);
	~BRD2File() {
		free(file_buf);
	}

	static bool verifyFormat(std::vector<char> &buf);

	private :
	template <int N>
	char * cmp(char * line, const char (&s) [N]) {
		return strncmp(line, s, N - 1) == 0 ? line + N - 1 : nullptr;
	}
};
