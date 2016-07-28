#pragma once

#include "BRDFile.h"

struct BVRFile : public BRDFile {
	BVRFile(const char *buf, size_t buffer_size);
	~BVRFile() {
		free(file_buf);
	}

	static bool verifyFormat(const char *buf, size_t buffer_size);
};
