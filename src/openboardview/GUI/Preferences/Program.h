#ifndef _PREFERENCES_PROGRAM_H_
#define _PREFERENCES_PROGRAM_H_

//#include "BoardView.h"
#include "UI/Keyboard/KeyBindings.h"
#include "GUI/Config.h"
#include "confparse.h"

class BoardView;

namespace Preferences {

class Program {
private:
	bool shown = false;
	KeyBindings	&keybindings;
	Confparse &obvconfig;
	Config &config;
	BoardView &boardView;
public:
	Program(KeyBindings &keybindings, Confparse &obvconfig, Config &config, BoardView &boardView);

	void menuItem();
	void render();
};

} // namespace Preferences

#endif
