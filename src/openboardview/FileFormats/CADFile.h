#pragma once

#include "BRDFile.h"

struct CADFile : public BRDFile {
	CADFile(std::vector<char> &buf);
	~CADFile() {
		free(file_buf);
	}
	private:
		void gen_outline();
};
