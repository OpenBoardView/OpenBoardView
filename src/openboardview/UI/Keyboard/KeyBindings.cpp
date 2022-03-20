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

KeyBindings::KeyBindings() {
	for (ImGuiKey key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++) {
		nameToKey.insert({ImGui::GetKeyName(key), key});
	}

	reset();
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
	keybindings["Quit"] = {KeyBinding(ImGuiKey_Q, {ImGuiKey_ModSuper})};
	keybindings["Open"] = {KeyBinding(ImGuiKey_O, {ImGuiKey_ModSuper})};
	keybindings["Search"] = {KeyBinding(ImGuiKey_F, {ImGuiKey_ModSuper}), KeyBinding(ImGuiKey_Slash)};
#else
	keybindings["Quit"] = {KeyBinding(ImGuiKey_Q, {ImGuiKey_ModCtrl})};
	keybindings["Open"] = {KeyBinding(ImGuiKey_O, {ImGuiKey_ModCtrl})};
	keybindings["Search"] = {KeyBinding(ImGuiKey_F, {ImGuiKey_ModCtrl}), KeyBinding(ImGuiKey_Slash)};
#endif
	keybindings["CloseDialog"] = {KeyBinding(ImGuiKey_Escape)};
	keybindings["Validate"] = {KeyBinding(ImGuiKey_Enter, {ImGuiKey_ModShift})};
	keybindings["Accept"] = {KeyBinding(ImGuiKey_Enter)};

	keybindings["Flip"] = {KeyBinding(ImGuiKey_Space)};
	keybindings["Mirror"] = {KeyBinding(ImGuiKey_M)};
	keybindings["RotateCW"] = {KeyBinding(ImGuiKey_R), KeyBinding(ImGuiKey_Period), KeyBinding(ImGuiKey_KeypadDecimal)};
	keybindings["RotateCCW"] = {KeyBinding(ImGuiKey_Comma), KeyBinding(ImGuiKey_Keypad0)};
	keybindings["ZoomIn"] = {KeyBinding(ImGuiKey_KeypadAdd), KeyBinding(ImGuiKey_Equal)}; // Due to config parsing quirk, = must be specified last
	keybindings["ZoomOut"] = {KeyBinding(ImGuiKey_Minus), KeyBinding(ImGuiKey_KeypadSubtract)};

	keybindings["PanDown"] = {KeyBinding(ImGuiKey_S), KeyBinding(ImGuiKey_Keypad2)};
	keybindings["PanUp"] = {KeyBinding(ImGuiKey_W), KeyBinding(ImGuiKey_Keypad8)};
	keybindings["PanLeft"] = {KeyBinding(ImGuiKey_A), KeyBinding(ImGuiKey_Keypad4)};
	keybindings["PanRight"] = {KeyBinding(ImGuiKey_D), KeyBinding(ImGuiKey_Keypad6)};
	keybindings["Center"] = {KeyBinding(ImGuiKey_X), KeyBinding(ImGuiKey_Keypad5)};


	keybindings["InfoPanel"] = {KeyBinding(ImGuiKey_I)};
	keybindings["NetList"] = {KeyBinding(ImGuiKey_L)};
	keybindings["PartList"] = {KeyBinding(ImGuiKey_K)};
	keybindings["TogglePins"] = {KeyBinding(ImGuiKey_P)};
	keybindings["Clear"] = {KeyBinding(ImGuiKey_Escape)};
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
			auto keyNames = split_string(kbs, modifierSeparator);
			if (keyNames.empty())
				continue;

			std::vector<ImGuiKey> keys;
			keys.reserve(keyNames.size());

			// Parse key names to ImGui keys
			std::transform(keyNames.begin(), keyNames.end(), std::back_inserter(keys), [this](std::string &keyName) -> ImGuiKey {
				// Replace some serialization-compatible keys with the ImGui-compatible name
				auto deserializedNameIt = deserializeName.find(keyName);
				if (deserializedNameIt != deserializeName.end()) {
					keyName = deserializedNameIt->second;
				}

				auto key = nameToKey.find(keyName);
				if (key == nameToKey.end()) {
					SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unknown key name: %s", keyName.c_str());
					return ImGuiKey_None;
				} else {
					return key->second;
				}
			});

			std::vector<ImGuiKey> modifiers(keys.begin(), keys.end()-1);

			bindings.push_back({keys.back(), modifiers});
		}
		keybinding.second = bindings;
	}

	writeToConfig(obvconfig);
}

void KeyBindings::writeToConfig(Confparse &obvconfig) {
	for (auto &keybinding : keybindings) {
		std::string line;
		for (auto &kbs : keybinding.second) {
			std::vector<ImGuiKey> keys = kbs.getModifiers();
			keys.push_back(kbs.getKey());

			std::string keyNames;

			for (const auto &key: keys) {
				std::string keyName = ImGui::GetKeyName(key);

				// Replace some keys with a serialization-compatible variant
				auto serializedNameIt = serializeName.find(keyName);
				if (serializedNameIt != serializeName.end()) {
					keyName = serializedNameIt->second;
				}

				if (!keyNames.empty()) {
					keyNames += modifierSeparator;
				}
				keyNames += keyName;
			}

			if (!line.empty())
				line += bindingSeparator;
			line += keyNames;
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
