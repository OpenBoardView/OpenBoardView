#include "KeyBinding.h"

#include <algorithm>
#include <iostream>

#include "imgui/imgui.h"

KeyBinding::KeyBinding(const SDL_Keycode keycode) : KeyBinding(keycode, {}) {
}

KeyBinding::KeyBinding(const SDL_Keycode keycode, std::vector<KeyModifier> modifiers) : keycode(keycode), modifiers(modifiers) {
}

bool KeyBinding::isPressed() const {
	auto scancode = SDL_GetScancodeFromKey(keycode);
	return std::all_of(modifiers.begin(), modifiers.end(), [](const KeyModifier &keyModifier){return keyModifier.isPressed();}) && ImGui::IsKeyPressed(scancode);
}

SDL_Keycode KeyBinding::getKeycode() const {
	return keycode;
}

std::vector<KeyModifier> KeyBinding::getModifiers() const {
	return modifiers;
}
