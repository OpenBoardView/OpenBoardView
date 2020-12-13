#include "KeyModifiers.h"

#include <algorithm>

#if __cplusplus < 201703L
constexpr const std::array<SDL_Keycode, 8> KeyModifiers::SDLKModifiers;
#endif

KeyModifiers::KeyModifiers() {
	std::transform(keyModifiers.begin(), keyModifiers.end(), std::inserter(nameToKeyModifier, nameToKeyModifier.begin()), [](const KeyModifier &keyModifier){
		return std::pair<std::string, KeyModifier>{keyModifier.name, keyModifier};
	});
}

const KeyModifier &KeyModifiers::fromName(const std::string &name) const {
	return nameToKeyModifier.at(name);
}

std::vector<KeyModifier> KeyModifiers::pressed() const {
	std::vector<KeyModifier> pressed;
	for (auto &keyModifier: keyModifiers) {
		if (keyModifier.isPressed()) {
			pressed.push_back(keyModifier);
		}
	}
	return pressed;
}

bool KeyModifiers::isSDLKModifier(SDL_Keycode kc) const {
	return std::find(SDLKModifiers.begin(), SDLKModifiers.end(), kc) != std::end(SDLKModifiers);
}
