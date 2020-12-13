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
	bool close_button_not_clicked = true;
	auto &io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), 0, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Keyboard Preferences", &close_button_not_clicked, ImGuiWindowFlags_AlwaysAutoResize)) {
		shown = false;

		// Find how many columns we need to show all the keybindings
		auto maxbindings = std::max_element(keybindings.keybindings.begin(), keybindings.keybindings.end(),
							[](const std::pair<std::string, std::vector<KeyBinding>> &a, const std::pair<std::string, std::vector<KeyBinding>> &b){ // C++14 has auto lambda argument deduction, would make it cleaner…
								return a.second.size() < b.second.size();});
		// total number of columns: name + keybindings + add button
		auto colcount = 1 + (maxbindings != keybindings.keybindings.end() ? maxbindings->second.size() : 0) + 1;

		if (ImGui::BeginTable("KeyBindings", colcount,
			ImGuiTableFlags_ColumnsWidthFixed // size according to content
			| ImGuiTableFlags_RowBg // alternating background colors
			| ImGuiTableFlags_BordersH
			| ImGuiTableFlags_BordersV)) {

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

				// Add new binding after pressing add button, keep that after showing all bindings
				if (addingName == kbs.first) {
					// Check if a key is pressed
					for (int i = 0; i < IM_ARRAYSIZE(io.KeysDown); i++) {
						if (io.KeysDown[i]) {
							auto scancode = static_cast<SDL_Scancode>(i);
							SDL_Keycode kc = SDL_GetKeyFromScancode(scancode);

							// Ignore modifier keys, cannot be a final key
							if (!keybindings.keyModifiers.isSDLKModifier(kc)) {
								addingScancode = scancode;
								addingBinding = KeyBinding(kc);
							}
						}
					}
					// Get the modifier keys pressed
					addingBinding.modifiers = keybindings.keyModifiers.pressed();

					// Add binding once non-modifier key is released
					if (addingScancode != SDL_SCANCODE_UNKNOWN && ImGui::IsKeyReleased(addingScancode)) {
						bindings.push_back(addingBinding);

						addingScancode = SDL_SCANCODE_UNKNOWN;
						addingName = {};
						addingBinding = {};
					}
				}

				ImGui::TableSetColumnIndex(colindex);
				if (addingName == kbs.first) {
					// Adding a new key binding
					auto keys = addingBinding.to_string();
					if (ImGui::SmallButton(("X##adding" + kbs.first).c_str())) {
						// Stop adding new keybinding
						addingScancode = SDL_SCANCODE_UNKNOWN;
						addingName = std::string{};
						addingBinding = KeyBinding{};
					}
					ImGui::SameLine();
					if (!keys.empty()) {
						// Show current pressed keys
						ImGui::Text("%s", keys.c_str());
					} else {
						// Or a message indicating to press a key
						ImGui::Text("%s", "<Press a key>");
					}
				} else {
					// Show add button
					if (ImGui::SmallButton(("Add##" + kbs.first).c_str())) {
						addingName = kbs.first;
					}
				}
			}

			ImGui::EndTable();
		}

		ImGui::Text("Note: %c and %c cannot be used as key bindings.", keybindings.bindingSeparator, keybindings.modifierSeparator);

		if (ImGui::Button("Save")) {
			keybindings.writeToConfig(obvconfig);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel") || keybindings.isPressed("CloseDialog") || !close_button_not_clicked) {
			keybindings.readFromConfig(obvconfig);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Default")) {
			keybindings.reset();
		}

		ImGui::EndPopup();
	}

	if (!close_button_not_clicked) { // modal title bar close button clicked
		printf("Close\n");
		keybindings.readFromConfig(obvconfig);
	}

	if (shown) {
		ImGui::OpenPopup("Keyboard Preferences");
	}
}

} // namespace Preferences
