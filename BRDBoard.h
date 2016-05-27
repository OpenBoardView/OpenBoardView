#pragma once

#include "BRDFile.h"
#include "Board.h"
#include <vector>
#include <string.h>


class BRDBoard :
	public Board
{
public:
	BRDBoard(const BRDFile* boardFile);
	~BRDBoard();

	static bool equalNames(BRDNail x, BRDNail y) {
		return strcmp(x.net, y.net) == 0 ? true : false;
	}

	static bool cmpAlphabetically(BRDNail x, BRDNail y) {
		return std::char_traits<char>::compare(x.net, y.net, MAX_COMP_NAME_LENGTH) < 0 ? true : false;
	}

	BoardType getBoardType();

	std::vector<BRDPart> getParts();
	std::vector<BRDPin> getPins();
	std::vector<BRDNail> getNails();

	// Uniquely named Net elements only.
	std::vector<BRDNail> getUniqueNetList();

private:
	const BRDFile* m_file;

	std::vector<BRDPart> m_parts;
	std::vector<BRDPin> m_pins;
	std::vector<BRDNail> m_nails;

	std::vector<BRDNail> m_netUnique;
};

