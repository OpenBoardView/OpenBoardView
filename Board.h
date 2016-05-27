#pragma once

#include <vector>
#include "BRDFile.h"

#define MAX_COMP_NAME_LENGTH 128

enum BoardType {
	UNKNOWN = 0,
	BRD = 0x01,
	BDV = 0x02
};

class Board
{
public:
	virtual ~Board() {}

	virtual std::vector<BRDPart> getParts()=0;
	virtual std::vector<BRDPin> getPins()=0;
	virtual std::vector<BRDNail> getNails()=0;

	virtual std::vector<BRDNail> getUniqueNetList()=0;
	
	BoardType getBoardType() {
		return UNKNOWN;
	}
};

