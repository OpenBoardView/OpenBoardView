#pragma once

#include <array>
#include <string>

#include "FZFile.h"

class CAEFile : public FZFile {
protected:
	const std::array<uint32_t, 44> getKeyParity() const override;
	const std::string getKeyErrorMsg() const override;
};
