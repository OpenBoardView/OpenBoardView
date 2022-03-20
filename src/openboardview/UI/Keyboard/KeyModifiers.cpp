#include "KeyModifiers.h"

#include <algorithm>

#if __cplusplus < 201703L
constexpr const std::array<ImGuiKey, 12> KeyModifiers::modifiers;
constexpr const std::array<ImGuiKey, 4> KeyModifiers::handledModifiers;
#endif

KeyModifiers::KeyModifiers() {
}

bool KeyModifiers::isModifier(ImGuiKey key) const {
	return std::find(modifiers.begin(), modifiers.end(), key) != std::end(modifiers);
}

std::vector<ImGuiKey> KeyModifiers::pressed() const {
	std::vector<ImGuiKey> pressed;
	for (auto &keyModifier: handledModifiers) {
		// Modifiers are held down
		if (ImGui::IsKeyDown(keyModifier)) {
			pressed.push_back(keyModifier);
		}
	}
	return pressed;
}
