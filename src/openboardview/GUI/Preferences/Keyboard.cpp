#include "Keyboard.h"

#include "imgui/imgui.h"

#include <algorithm>
#include <iostream>
namespace Preferences {

Keyboard::Keyboard(KeyBindings &keybindings, Confparse &obvconfig) : keybindings(keybindings), obvconfig(obvconfig) {
}

void Keyboard::menuItem() {
	if (ImGui::MenuItem("Keyboard Preferences")) {
		shown = true;
	}
}

void Keyboard::render() {
	bool p_open = true;
	auto &io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), 0, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Keyboard Preferences", &p_open, ImGuiWindowFlags_AlwaysAutoResize)) {
		shown = false;
		was_open = true;

		// Find how many columns we need to show all the keybindings
		auto maxbindings = std::max_element(keybindings.keybindings.begin(), keybindings.keybindings.end(),
							[](const std::pair<std::string, std::vector<KeyBinding>> &a, const std::pair<std::string, std::vector<KeyBinding>> &b){ // C++14 has auto lambda argument deduction, would make it cleaner…
								return a.second.size() < b.second.size();});
		// total number of columns: name + keybindings + add button
		auto colcount = 1 + (maxbindings != keybindings.keybindings.end() ? maxbindings->second.size() : 0) + 1;

		if (ImGui::BeginTable("KeyBindings", colcount,
			ImGuiTableFlags_RowBg // alternating background colors
			| ImGuiTableFlags_BordersH
			| ImGuiTableFlags_BordersV)) {

			for (auto i = colcount; i > 0; i--) {
				ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed); // size according to content
			}

			for (auto &kbs : keybindings.keybindings) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%s", kbs.first.c_str()); // binding name


				auto &bindings = kbs.second; // reference because we add/delete from it below


				// Show all bindings, potentially newly added button will be shown next frame since we need to compute number of column again first
				auto colindex = 1;
				for (auto it = bindings.begin(); it != bindings.end();) {
					auto keys = it->to_string();

					ImGui::TableSetColumnIndex(colindex);
					if (ImGui::SmallButton(("X##" + kbs.first + std::to_string(colindex)).c_str())) {
						// Erase binding when clicking on X button
						it = bindings.erase(it);
					} else {
						// warning: iterator incremented here since it must not be incremented after reaffectation if binding is erased
						++it;
					}
					ImGui::SameLine();
					ImGui::Text("%s", keys.c_str());

					colindex++;
				}

				ImGui::TableSetColumnIndex(colindex);
				if (addingName == kbs.first) {
					// Adding a new key binding
					auto keys = addingBinding.to_string();
					if (ImGui::SmallButton(("X##adding" + kbs.first).c_str())) {
						// Stop adding new keybinding
						addingKey = ImGuiKey_None;
						addingName = std::string{};
						addingBinding = KeyBinding{};
					}
					bool x_button_clicked = ImGui::IsItemActive();

					ImGui::SameLine();
					if (!keys.empty()) {
						// Show current pressed keys
						ImGui::Text("%s", keys.c_str());
					} else {
						// Or a message indicating to press a key
						ImGui::Text("%s", "<Press a key>");
					}

					// Dot not attempt to add a key if the X button is being clicked
					// Prevent showing "MouseLeft" while clicking the button before releasing
					if (x_button_clicked) {
						continue;
					}
				} else {
					// Show add button
					if (ImGui::SmallButton(("Add##" + kbs.first).c_str())) {
						addingName = kbs.first;
					}
				}

				// Add new binding after pressing add button, keep that after showing all bindings
				if (addingName == kbs.first) {
					// Check if a key is pressed
					for (ImGuiKey key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key = (ImGuiKey)(key + 1)) {
						if (ImGui::IsKeyDown(key)) {
							// Ignore modifier keys, cannot be a final key
							if (!keybindings.keyModifiers.isModifier(key)) {
								addingKey = key;
								addingBinding = KeyBinding(key);
							}
						}
					}
					// Get the modifier keys pressed
					addingBinding.modifiers = keybindings.keyModifiers.pressed();

					// Add binding once non-modifier key is released
					if (addingKey != ImGuiKey_None && ImGui::IsKeyReleased(addingKey)) {
						bindings.push_back(addingBinding);

						addingKey = ImGuiKey_None;
						addingName = {};
						addingBinding = {};
					}
				}

			}

			ImGui::EndTable();
		}

		if (ImGui::Button("Save")) {
			keybindings.writeToConfig(obvconfig);
			ImGui::CloseCurrentPopup();
			was_open = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel") || keybindings.isPressed("CloseDialog")) {
			keybindings.readFromConfig(obvconfig);
			ImGui::CloseCurrentPopup();
			was_open = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Default")) {
			keybindings.reset();
		}

		ImGui::EndPopup();
	}

	if (!p_open) { // ImGui tells us the popup is closed
		if (was_open) {
			// We need to run this only once when the popup is closed with the popup title bar close button
			// If it was closed with our Save or Cancel button, we already did what we had to do and was_open is already false
			keybindings.readFromConfig(obvconfig);
			was_open = false;
		}
	}

	if (shown) {
		ImGui::OpenPopup("Keyboard Preferences");
	}
}

} // namespace Preferences
