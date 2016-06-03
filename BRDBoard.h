#pragma once

#include "Board.h"

#include <vector>
#include <string.h>
#include <memory>

using namespace std;


class BRDBoard :
	public Board
{
public:
	BRDBoard(const BRDFile * const boardFile);
	~BRDBoard();

    const BRDFile* m_file;

	EBoardType BoardType();

    SharedVector<Net> &Nets();
    SharedVector<Component> &Components();
    SharedVector<Pin> &Pins();
    SharedVector<Point> &OutlinePoints();

private:
    static const string kNetUnconnectedPrefix;

	static bool namesEqual(BRDNail lhs, BRDNail rhs) {
		return strcmp(lhs.net, rhs.net) == 0 ? true : false;
	}

	static bool cmpAlphabetically(BRDNail lhs, BRDNail rhs) {
		return char_traits<char>::
			compare(lhs.net, rhs.net, kBoardElementNameLength) < 0 ? true : false;
	}

    // Reading annotations for this board from json file.
    bool FetchPartAnnotations();

    SharedVector<Net> nets_;
    SharedVector<Component> components_;
    SharedVector<Pin> pins_;
    SharedVector<Point> outline_;
};

