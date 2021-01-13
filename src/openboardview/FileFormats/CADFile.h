#pragma once

#include "BRDFile.h"

struct CADFile : public BRDFile {
	CADFile(std::vector<char> &buf);
	~CADFile() {
		free(file_buf);
	}
	enum Block {
		Invalid,
		None,
		Parts,
		Pins,
		Nets,
		Vias
	};

	static bool verifyFormat(std::vector<char> &buf);
	private:
		void gen_outline();
};
