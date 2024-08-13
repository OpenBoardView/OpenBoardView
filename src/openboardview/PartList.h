#pragma once

#include "Board.h"
#include "UI/Keyboard/KeyBindings.h"

class PartList {
  public:
	PartList(KeyBindings &keyBindings, TcharStringCallback cbNetSelected);
	~PartList();

	void Draw(const char *title, bool *p_open, Board *board);

  private:
	KeyBindings	&keyBindings;
	TcharStringCallback cbNetSelected_;
};
