#ifndef _KEYBINDING_H_
#define _KEYBINDING_H_

#include <string>
#include <vector>

#include "SDL.h"
#include "imgui/imgui.h"

#include "KeyModifiers.h"

class KeyBinding {
private:
	SDL_Keycode keycode;
	std::vector<KeyModifier> modifiers;
public:
	KeyBinding(const SDL_Keycode keycode);
	KeyBinding(const SDL_Keycode keycode, std::vector<KeyModifier> modifiers);

	bool isPressed() const;
	SDL_Keycode getKeycode() const;
	std::vector<KeyModifier> getModifiers() const;
};

#endif
