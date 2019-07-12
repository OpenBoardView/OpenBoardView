#ifndef _KEYMODIFIER_H_
#define _KEYMODIFIER_H_

#include <string>

#include "imgui/imgui.h"

struct KeyModifier {
	std::string name;
	bool ImGuiIO::* value;

	bool isPressed() const;
};

#endif

