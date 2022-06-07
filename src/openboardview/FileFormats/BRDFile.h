#pragma once
#include "BRDFileBase.h"

#include <array>
#include <cstdlib>
#include <cstdint>
#include <vector>

class BRDFile : public BRDFileBase {
  public:
	BRDFile(std::vector<char> &buf);

	static bool verifyFormat(std::vector<char> &buf);

  private:
	static constexpr std::array<uint8_t, 4> signature = {{0x23, 0xe2, 0x63, 0x28}};
};
