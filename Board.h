#pragma once

#include <vector>
#include <functional>
#include <string>

#define MAX_COMP_NAME_LENGTH 128

#define EMPTY_STRING ""

typedef std::function<void(char*)> TcharStringCallback;

enum EBoardType {
	UNKNOWN = 0,
	BRD = 0x01,
	BDV = 0x02
};

struct BRDPoint {
    int x;
    int y;
};

struct BRDPart {
    char *name;
    char *annotation;
    int type;
    int end_of_pins;
};

struct BRDPin {
    BRDPoint pos;
    int probe;
    int part;
    char *net;
};

struct BRDNail {
    int probe;
    BRDPoint pos;
    int side;
    char *net;
};

struct Component {
    //BRD
};

class Board
{
public:
	virtual ~Board() {}

	virtual std::vector<BRDPart> Parts()=0;
	virtual std::vector<BRDPin> Pins()=0;
	virtual std::vector<BRDNail> Nails()=0;
    virtual std::vector<BRDPoint> OutlinePoints()=0;

	virtual std::vector<BRDNail> UniqueNetNails()=0;
	
	EBoardType BoardType() {
		return UNKNOWN;
	}
};

