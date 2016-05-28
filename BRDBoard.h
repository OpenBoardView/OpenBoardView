#pragma once

#include "Board.h"
#include "BRDFile.h"

#include <vector>
#include <string.h>


class BRDBoard :
	public Board
{
public:
	BRDBoard(const BRDFile* boardFile);
	~BRDBoard();

	EBoardType BoardType();

	std::vector<BRDPart> Parts();
	std::vector<BRDPin> Pins();
	std::vector<BRDNail> Nails();
    std::vector<BRDPoint> OutlinePoints();

	// Uniquely named Net elements only.
	std::vector<BRDNail> UniqueNetNails();

private:
	static bool equalNames(BRDNail x, BRDNail y) {
		return strcmp(x.net, y.net) == 0 ? true : false;
	}

	static bool cmpAlphabetically(BRDNail x, BRDNail y) {
		return std::char_traits<char>::
			compare(x.net, y.net, MAX_COMP_NAME_LENGTH) < 0 ? true : false;
	}

    static char* getNetName(BRDNail nail) {
        return nail.net;
    }

    bool FetchPartAnnotations();

	const BRDFile* m_file;

	std::vector<BRDPart> m_parts;
	std::vector<BRDPin> m_pins;
	std::vector<BRDNail> m_nails;
    std::vector<BRDPoint> m_points;

	std::vector<BRDNail> m_netUnique;
};

