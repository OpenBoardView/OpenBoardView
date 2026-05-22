#ifndef _HELP_CONTROLS_H_
#define _HELP_CONTROLS_H_

#include <string>

#include "UI/Keyboard/KeyBindings.h"

namespace Help {

class Controls {
private:
	bool shown = false;
	KeyBindings	&keybindings;
public:
	Controls(KeyBindings &keybindings);

	void menuItem();
	void render();
};

} // namespace Help
//
#endif
