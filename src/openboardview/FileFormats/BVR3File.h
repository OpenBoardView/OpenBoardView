#pragma once

#include "BRDFileBase.h"

struct BVR3File : public BRDFileBase {
	BVR3File(std::vector<char> &buf);

	static bool verifyFormat(std::vector<char> &buf);
};
