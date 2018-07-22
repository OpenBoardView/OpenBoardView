#pragma once

#include "BRDFile.h"

struct CADFile : public BRDFile {
	CADFile(std::vector<char> &buf);
	~CADFile() {
		free(file_buf);
	}
	static bool verifyFormat(std::vector<char> &buf);
	private:
		void gen_outline();
};
