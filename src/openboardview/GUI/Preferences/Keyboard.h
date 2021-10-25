#ifndef _PREFERENCES_KEYBOARD_H_
#define _PREFERENCES_KEYBOARD_H_

#include "UI/Keyboard/KeyBindings.h"
namespace Preferences {

class Keyboard {
private:
	bool shown = false;
	KeyBindings	&keybindings;
	Confparse &obvconfig;
	std::string addingName;
	KeyBinding addingBinding;
	SDL_Scancode addingScancode = SDL_SCANCODE_UNKNOWN;
public:
	Keyboard(KeyBindings &keybindings, Confparse &obvconfig);

	void menuItem(const char * name = nullptr);
	void render(const char * name = nullptr);
};

} // namespace Preferences

#endif
