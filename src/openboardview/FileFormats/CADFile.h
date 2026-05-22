#pragma once

#include "BRDFileBase.h"

struct CADFile : public BRDFileBase {
	CADFile(std::vector<char> &buf);
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
