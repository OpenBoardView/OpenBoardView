#pragma once

#include "BRDFile.h"

struct BVRFile : public BRDFile {
	BVRFile(std::vector<char> &buf);
	~BVRFile() {
		free(file_buf);
	}

	static bool verifyFormat(std::vector<char> &buf);
};
