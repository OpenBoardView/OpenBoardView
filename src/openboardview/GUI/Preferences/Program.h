#ifndef _PREFERENCES_PROGRAM_H_
#define _PREFERENCES_PROGRAM_H_

//#include "BoardView.h"
#include "UI/Keyboard/KeyBindings.h"
#include "GUI/Config.h"
#include "confparse.h"
#include "BoardAppearance.h"

class BoardView;

namespace Preferences {

class Program {
private:
	bool shown = false;
	bool wasOpen = false;
	KeyBindings	&keybindings;
	Confparse &obvconfig;
	Config &config;
	Config configCopy;
	BoardView &boardView;

	BoardAppearance boardAppearance{obvconfig, config};
public:
	Program(KeyBindings &keybindings, Confparse &obvconfig, Config &config, BoardView &boardView);

	void menuItem();
	void render();
};

} // namespace Preferences

#endif
