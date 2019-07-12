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
private:
	std::unordered_map<std::string, std::vector<KeyBinding>> keybindings;
	KeyModifiers keyModifiers;
public:
	KeyBindings();

	bool isPressed(const std::string &name) const;

//	void add(const std::string &name);
//	void clear(const std::string &name);
	void reset();

	void readFromConfig(Confparse &obvconfig);
	void writeToConfig(Confparse &obvconfig);
};

#endif
