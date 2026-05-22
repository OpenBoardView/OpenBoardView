#pragma once

#include "BRDFileBase.h"
struct BRD2File : public BRDFileBase {
	BRD2File(std::vector<char> &buf);

	static bool verifyFormat(std::vector<char> &buf);
};
