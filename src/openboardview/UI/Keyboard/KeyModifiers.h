#ifndef _KEYMODIFIERS_H_
#define _KEYMODIFIERS_H_

#include <array>
#include <unordered_map>
#include <vector>

#include "imgui/imgui.h"

class KeyModifiers {
private:
	// All modifiers key that should not be treated as regular keys
	constexpr static const std::array<ImGuiKey, 12> modifiers{{
		ImGuiKey_ModCtrl,
		ImGuiKey_ModShift,
		ImGuiKey_ModAlt,
		ImGuiKey_ModSuper,
		ImGuiKey_LeftCtrl,
		ImGuiKey_LeftShift,
		ImGuiKey_LeftAlt,
		ImGuiKey_LeftSuper,
		ImGuiKey_RightCtrl,
		ImGuiKey_RightShift,
		ImGuiKey_RightAlt,
		ImGuiKey_RightSuper,
	}};

	// Modifier keys to handle as modifiers
	// Only ImGuiKey_Mod* are used here in order to handle both Left and Right modifiers the same way
	// If handling them separately is required, ImGuiKey_Left* and ImGuiKey_Right* should be specified instead
	constexpr static const std::array<ImGuiKey, 4> handledModifiers{{
		ImGuiKey_ModCtrl,
		ImGuiKey_ModShift,
		ImGuiKey_ModAlt,
		ImGuiKey_ModSuper,
	}};

public:
	KeyModifiers();

	bool isModifier(ImGuiKey key) const;
	std::vector<ImGuiKey> pressed() const;
};

#endif
