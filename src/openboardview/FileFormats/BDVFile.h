#pragma once

#include "BRDFileBase.h"

struct BDVFile : public BRDFileBase {
	BDVFile(std::vector<char> &buf);

	static bool verifyFormat(std::vector<char> &buf);
};
