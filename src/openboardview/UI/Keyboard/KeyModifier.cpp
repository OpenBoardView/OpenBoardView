#include "KeyModifier.h"

bool KeyModifier::isPressed() const {
	auto io = ImGui::GetIO();
	return io.*value;
}
