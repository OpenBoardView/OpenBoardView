#pragma once

#include "BRDFile.h"

struct BDVFile : public BRDFile {
	BDVFile(const char *buf, size_t buffer_size);
	~BDVFile() {
		free(file_buf);
	}
};
