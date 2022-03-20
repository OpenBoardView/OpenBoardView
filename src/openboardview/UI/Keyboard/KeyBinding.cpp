#include "KeyBinding.h"

#include <algorithm>
#include <iostream>

#include "imgui/imgui.h"

KeyBinding::KeyBinding() {
}

KeyBinding::KeyBinding(const ImGuiKey key) : KeyBinding(key, {}) {
}

KeyBinding::KeyBinding(const ImGuiKey key, std::vector<ImGuiKey> modifiers) : key(key), modifiers(modifiers) {
}

bool KeyBinding::isPressed() const {
	// Modifiers are held down while non-modifier key is pressed once
	return std::all_of(modifiers.begin(), modifiers.end(), [](const ImGuiKey &keyModifier){return (keyModifier != ImGuiKey_None) && ImGui::IsKeyDown(keyModifier);}) && (key != ImGuiKey_None) && ImGui::IsKeyPressed(key);
}

ImGuiKey KeyBinding::getKey() const {
	return key;
}

std::vector<ImGuiKey> KeyBinding::getModifiers() const {
	return modifiers;
}

std::string KeyBinding::to_string() const {
	std::string keys = ImGui::GetKeyName(this->getKey());

	auto modifiers = this->getModifiers();
	for (auto im = modifiers.rbegin(); im != modifiers.rend(); ++im ) {
		keys = std::string(ImGui::GetKeyName(*im)) + "+" + keys;
	}

	return keys;
}
