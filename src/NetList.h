#pragma once

#include "Board.h"

#include <vector>

class NetList {

  public:
	NetList(TcharStringCallback cbNetSelected);
	~NetList();

	void Draw(const char *title, bool *p_open, Board *board);

  private:
	TcharStringCallback cbNetSelected_;
};
