#pragma once
#include "BRDFileBase.h"

#include <deque>
#include <string>
#include <vector>

// Tebo-ICT View (*.TVW) board files.
//
// The binary .tvw format was reverse-engineered by Pavel Kovalenko
// (github.com/nitrocaster/eagleview, MIT). The actual container parser lives
// in FileFormats/tebo/ (Tebo::Board); this class adapts the parsed result to
// OpenBoardView's BRDFileBase model.
class TVWFile : public BRDFileBase {
  public:
	TVWFile(std::vector<char> &buf);

	static bool verifyFormat(std::vector<char> &buf);

  private:
	// stable backing storage for the const char* names/nets handed to BRDFileBase
	std::deque<std::string> pool_;
	const char *intern(const std::string &s);
};
