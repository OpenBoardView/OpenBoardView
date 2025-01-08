#include "CAEFile.h"

#include <array>
#include <string>

const std::string CAEFile::getKeyErrorMsg() const {
	return "Invalid CAE key\nCAE Key:\n";
}

const std::array<uint32_t, 44> CAEFile::getKeyParity() const {
	return {{1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0}};
}
