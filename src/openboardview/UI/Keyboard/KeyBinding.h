#ifndef _KEYBINDING_H_
#define _KEYBINDING_H_

#include <string>
#include <vector>

#include "imgui/imgui.h"

#include "KeyModifiers.h"

class KeyBinding {
private:
	ImGuiKey key = ImGuiKey_None;
public:
	std::vector<ImGuiKey> modifiers;

	KeyBinding();
	KeyBinding(const ImGuiKey key);
	KeyBinding(const ImGuiKey key, std::vector<ImGuiKey> modifiers);

	bool isPressed() const;
	ImGuiKey getKey() const;
	std::vector<ImGuiKey> getModifiers() const;
	std::string to_string() const;
};

#endif
