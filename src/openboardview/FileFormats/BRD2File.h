#pragma once

#include "BRDFile.h"
struct BRD2File : public BRDFile {
	BRD2File(std::vector<char> &buf);
	~BRD2File() {
		free(file_buf);
	}

	static bool verifyFormat(std::vector<char> &buf);
};
