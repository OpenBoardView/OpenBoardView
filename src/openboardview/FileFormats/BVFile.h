#pragma once

#include "BRDFileBase.h"

#include <deque>
#include <string>
#include <vector>

// Legacy BoardView ".bv" files: a Microsoft Jet 4 (Access 2000+) database
// holding three tables, Layout (board outline), Pin and Nail.
struct BVFile : public BRDFileBase {
	BVFile(std::vector<char> &buf);

	static bool verifyFormat(std::vector<char> &buf);

  private:
	// Owns the strings that parts/pins/nails point to. A deque never moves its
	// elements on push_back, so the c_str() pointers stay valid.
	std::deque<std::string> strings;
	const char *keep(const std::string &s);
};
