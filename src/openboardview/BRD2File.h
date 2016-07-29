#pragma once

#include "BRDFile.h"
struct BRD2File : public BRDFile {
	BRD2File(const char *buf, size_t buffer_size);
	~BRD2File() {
		free(file_buf);
	}

	static bool verifyFormat(const char *buf, size_t buffer_size);
};
