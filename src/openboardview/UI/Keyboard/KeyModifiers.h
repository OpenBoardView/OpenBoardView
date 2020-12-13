#ifndef _KEYMODIFIERS_H_
#define _KEYMODIFIERS_H_

#include <array>
#include <unordered_map>
#include <vector>

#include "SDL.h"
#include "imgui/imgui.h"

#include "KeyModifier.h"

class KeyModifiers {
private:
	const std::array<KeyModifier, 4> keyModifiers{{
		{"Ctrl", &ImGuiIO::KeyCtrl},
		{"Shift", &ImGuiIO::KeyShift},
		{"Alt", &ImGuiIO::KeyAlt},
		{"Super", &ImGuiIO::KeySuper}
	}};

	constexpr static const std::array<SDL_Keycode, 8> SDLKModifiers{
		SDLK_LCTRL,
		SDLK_RCTRL,
		SDLK_LSHIFT,
		SDLK_RSHIFT,
		SDLK_LALT,
		SDLK_RALT,
		SDLK_LGUI,
		SDLK_RGUI
	};

	std::unordered_map<std::string, KeyModifier> nameToKeyModifier;

public:
	KeyModifiers();

	const KeyModifier &fromName(const std::string &name) const;
	std::vector<KeyModifier> pressed() const;
	bool isSDLKModifier(SDL_Keycode kc) const;
};

#endif
