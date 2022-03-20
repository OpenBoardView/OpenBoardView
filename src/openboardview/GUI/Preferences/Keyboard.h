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
	ImGuiKey addingKey = ImGuiKey_None;
public:
	Keyboard(KeyBindings &keybindings, Confparse &obvconfig);

	void menuItem();
	void render();
};

} // namespace Preferences

#endif
