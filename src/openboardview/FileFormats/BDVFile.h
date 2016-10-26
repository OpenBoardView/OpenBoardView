#pragma once

#include "BRDFile.h"

struct BDVFile : public BRDFile {
	BDVFile(std::vector<char> &buf);
	~BDVFile() {
		free(file_buf);
	}

	static bool verifyFormat(std::vector<char> &buf);
};
