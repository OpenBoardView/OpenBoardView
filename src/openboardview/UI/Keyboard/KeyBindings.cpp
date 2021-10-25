#include "KeyBindings.h"

#include <algorithm>
#include <iostream>

#include "SDL.h"
#include "imgui/imgui.h"

#include "utils.h"

const std::unordered_map<std::string, std::string> KeyBindings::serializeName = {
	{"|", "Pipe"},
	{"~", "Tilde"},
	{"=", "Equals"}
};

const std::unordered_map<std::string, std::string> KeyBindings::deserializeName = {
	{"Pipe", "|"},
	{"Tilde", "~"},
	{"Equals", "="}
};

KeyBindings::KeyBindings(std::vector<std::string> const * actions) {
	if (actions) {
		for (auto && it : *actions) {
			keybindings[it] = { };
		}
	} else {
		reset();
	}
}

bool KeyBindings::isPressed(const std::string &name) const {
	auto kb = keybindings.find(name);
	if (kb == keybindings.end()) {
		return false;
	}
	return std::any_of(kb->second.begin(), kb->second.end(), [](const KeyBinding &kb){return kb.isPressed();});
}

void KeyBindings::reset() {
#ifdef __APPLE__
	keybindings["Quit"] = {KeyBinding(SDLK_q, {keyModifiers.fromName("Super")})};
	keybindings["Open"] = {KeyBinding(SDLK_o, {keyModifiers.fromName("Super")})};
	keybindings["Search"] = {KeyBinding(SDLK_f, {keyModifiers.fromName("Super")}), KeyBinding(SDLK_SLASH)};
#else
	keybindings["Quit"] = {KeyBinding(SDLK_q, {keyModifiers.fromName("Ctrl")})};
	keybindings["Open"] = {KeyBinding(SDLK_o, {keyModifiers.fromName("Ctrl")})};
	keybindings["Search"] = {KeyBinding(SDLK_f, {keyModifiers.fromName("Ctrl")}), KeyBinding(SDLK_SLASH)};
#endif
	keybindings["CloseDialog"] = {KeyBinding(SDLK_ESCAPE)};
	keybindings["Validate"] = {KeyBinding(SDLK_RETURN, {keyModifiers.fromName("Shift")})};
	keybindings["Accept"] = {KeyBinding(SDLK_RETURN)};

	keybindings["Flip"] = {KeyBinding(SDLK_SPACE)};
	keybindings["Mirror"] = {KeyBinding(SDLK_m)};
	keybindings["RotateCW"] = {KeyBinding(SDLK_r), KeyBinding(SDLK_PERIOD), KeyBinding(SDLK_KP_PERIOD)};
	keybindings["RotateCCW"] = {KeyBinding(SDLK_COMMA), KeyBinding(SDLK_KP_0)};
	keybindings["ZoomIn"] = {KeyBinding(SDLK_KP_PLUS), KeyBinding(SDLK_EQUALS)}; // Due to config parsing quirk, = must be specified last
	keybindings["ZoomOut"] = {KeyBinding(SDLK_MINUS), KeyBinding(SDLK_KP_MINUS)};

	keybindings["PanDown"] = {KeyBinding(SDLK_s), KeyBinding(SDLK_KP_2)};
	keybindings["PanUp"] = {KeyBinding(SDLK_w), KeyBinding(SDLK_KP_8)};
	keybindings["PanLeft"] = {KeyBinding(SDLK_a), KeyBinding(SDLK_KP_4)};
	keybindings["PanRight"] = {KeyBinding(SDLK_d), KeyBinding(SDLK_KP_6)};
	keybindings["Center"] = {KeyBinding(SDLK_x), KeyBinding(SDLK_KP_5)};


	keybindings["InfoPanel"] = {KeyBinding(SDLK_i)};
	keybindings["NetList"] = {KeyBinding(SDLK_l)};
	keybindings["PartList"] = {KeyBinding(SDLK_k)};
	keybindings["TogglePins"] = {KeyBinding(SDLK_p)};
	keybindings["Clear"] = {KeyBinding(SDLK_ESCAPE)};

	keybindings["Fullscreen"] = { KeyBinding(SDLK_f) };
}


// Serialization/deserialization use '|' to specify multiple bindings and '~' to combine with modifiers, so they cannot be used in key names
void KeyBindings::readFromConfig(Confparse &obvconfig) {
	for (auto &keybinding : keybindings) {
		auto confline = obvconfig.ParseStr(("KeyBinding" + keybinding.first).c_str(), nullptr);
		if (confline == nullptr)
			continue;

		// Multiple bindings for a single function are combined with bindingSeparator '|'
		std::vector<KeyBinding> bindings;
		for (auto &kbs : split_string(confline, bindingSeparator)) {
			//Modifiers combined with modifierSeparator '~' to key, key always specified last
			auto keys = split_string(kbs, modifierSeparator);
			if (keys.empty())
				continue;
			std::vector<std::string> modifiers(keys.begin(), keys.end()-1);
			std::vector<KeyModifier> vkeyModifiers;

			vkeyModifiers.reserve(modifiers.size());
			std::transform(modifiers.begin(), modifiers.end(), std::back_inserter(vkeyModifiers), [this](const std::string &modifier){return keyModifiers.fromName(modifier);});

			std::string key = keys.back();
			// Replace some serialization-compatible keys with the SDL-compatible name
			auto deserializedNameIt = deserializeName.find(key);
			if (deserializedNameIt != deserializeName.end()) {
				key = deserializedNameIt->second;
			}

			auto sdlKey = SDL_GetKeyFromName(key.c_str());

			bindings.push_back({sdlKey, vkeyModifiers});
		}
		keybinding.second = bindings;
	}

	writeToConfig(obvconfig);
}

void KeyBindings::writeToConfig(Confparse &obvconfig) {
	for (auto &keybinding : keybindings) {
		std::string line;
		for (auto &kbs : keybinding.second) {
			std::string binding = SDL_GetKeyName(kbs.getKeycode());

			// Replace some keys with a serialization-compatible variant
			auto serializedNameIt = serializeName.find(binding);
			if (serializedNameIt != serializeName.end()) {
				binding = serializedNameIt->second;
			}

			// Prepend modifiers so that key is always last, reverse to keep in same order
			auto modifiers = kbs.getModifiers();
			for (auto im = modifiers.rbegin(); im != modifiers.rend(); ++im ) {
				binding = im->name + modifierSeparator + binding;
			}

			if (!line.empty())
				line += bindingSeparator;
			line += binding;
		}
		obvconfig.WriteStr(("KeyBinding" + keybinding.first).c_str(), line.c_str());
	}
}

std::string KeyBindings::getKeyNames(const std::string &bindname) const {
	std::string str{};
	auto kb = keybindings.find(bindname);
	if (kb != keybindings.end()) {
		for (const auto &kbs : kb->second) {
			if (!str.empty())
				str += " ";
			str += "<" + kbs.to_string() + ">";
		}
	}
	return str;
}
