#pragma once

#include "BRDFileBase.h"

struct BVRFile : public BRDFileBase {
	BVRFile(std::vector<char> &buf);

	static bool verifyFormat(std::vector<char> &buf);
};
