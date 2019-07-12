#include "KeyModifiers.h"

#include <algorithm>

KeyModifiers::KeyModifiers() {
	std::transform(keyModifiers.begin(), keyModifiers.end(), std::inserter(nameToKeyModifier, nameToKeyModifier.begin()), [](const KeyModifier &keyModifier){
		return std::pair<std::string, KeyModifier>{keyModifier.name, keyModifier};
	});
}

const KeyModifier &KeyModifiers::fromName(const std::string &name) const {
	return nameToKeyModifier.at(name);
}

