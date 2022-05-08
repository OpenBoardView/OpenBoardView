#pragma once

#include "BRDFile.h"

struct BVR3File : public BRDFile {
	BVR3File(std::vector<char> &buf);
	~BVR3File() {
		free(file_buf);
	}

	static bool verifyFormat(std::vector<char> &buf);
};
