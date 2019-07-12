#ifndef _KEYMODIFIERS_H_
#define _KEYMODIFIERS_H_

#include <array>
#include <unordered_map>

#include "imgui/imgui.h"

#include "KeyModifier.h"

class KeyModifiers {
private:
	std::array<KeyModifier, 4> keyModifiers{{
		{"Ctrl", &ImGuiIO::KeyCtrl},
		{"Shift", &ImGuiIO::KeyShift},
		{"Alt", &ImGuiIO::KeyAlt},
		{"Super", &ImGuiIO::KeySuper}
	}};

	std::unordered_map<std::string, KeyModifier> nameToKeyModifier;

public:
	KeyModifiers();

	const KeyModifier &fromName(const std::string &name) const;

};

#endif
