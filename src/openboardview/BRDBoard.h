#pragma once

#include "Board.h"

#include <memory>
#include <string.h>
#include <vector>

using namespace std;

class BRDBoard : public Board {
  public:
	BRDBoard(const BRDFileBase *const boardFile);
	~BRDBoard();

	const BRDFileBase *m_file;

	EBoardType BoardType();

	SharedVector<Net> &Nets();
	SharedVector<Component> &Components();
	SharedVector<Pin> &Pins();
	SharedVector<Point> &OutlinePoints();

  private:
	static const string kNetUnconnectedPrefix;
	static const string kComponentDummyName;

	SharedVector<Net> nets_;
	SharedVector<Component> components_;
	SharedVector<Pin> pins_;
	SharedVector<Point> outline_;
};
