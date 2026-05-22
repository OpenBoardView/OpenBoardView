#pragma once

#include "Board.h"
#include "UI/Keyboard/KeyBindings.h"

class NetList {
  public:
	NetList(KeyBindings &keyBindings, TcharStringCallback cbNetSelected);
	~NetList();

	void Draw(const char *title, bool *p_open, Board *board);

  private:
	KeyBindings	&keyBindings;
	TcharStringCallback cbNetSelected_;
};
