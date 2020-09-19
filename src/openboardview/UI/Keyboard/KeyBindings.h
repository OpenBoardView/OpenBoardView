#ifndef _KEYBINDINGS_H_
#define _KEYBINDINGS_H_

#include <string>
#include <vector>
#include <unordered_map>

#include "imgui/imgui.h"
#include "KeyBinding.h"
#include "KeyModifiers.h"
#include "confparse.h"

class KeyBindings {
public:
	std::unordered_map<std::string, std::vector<KeyBinding>> keybindings;
	KeyModifiers keyModifiers;

	static const char bindingSeparator = '|';
	static const char modifierSeparator = '~';

	KeyBindings();

	bool isPressed(const std::string &name) const;

	void reset();

	void readFromConfig(Confparse &obvconfig);
	void writeToConfig(Confparse &obvconfig);

	std::string getKeyNames(const std::string &bindname) const;
};

#endif
