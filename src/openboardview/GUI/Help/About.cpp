#include "About.h"

#include <string>

#include "imgui/imgui.h"

#include "GUI/DPI.h"
#include "version.h"

namespace Help {

const std::string About::OBV_LICENSE_TEXT_NONWRAPPED{removeSingleLineFeed(OBV_LICENSE_TEXT)};

std::string About::removeSingleLineFeed(std::string str) {
	for (auto it = str.begin(); it != str.end(); it++) {
		if (*it == '\n') {
			if (*(it + 1) == '\n') { // Two consecutive line feeds encountered
				while (*++it == '\n'); // Skip all subsequent line feeds
			} else {
				*it = ' '; // Single consecutive line feed replaced with whitespace
			}
		}
	}
	return str;
}

About::About(KeyBindings &keybindings) : keybindings(keybindings) {
}

void About::menuItem() {
	if (ImGui::MenuItem("About")) {
		shown = true;
	}
}

void About::render() {
	bool p_open = true;
	auto &io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), 0, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("About", &p_open, ImGuiWindowFlags_AlwaysAutoResize)) {
		shown = false;

		ImGui::Text("%s %s", OBV_NAME, OBV_VERSION);
		ImGui::Text("Build %s %s", OBV_BUILD, __DATE__ " " __TIME__);
		ImGui::TextLinkOpenURL(OBV_URL);
		ImGui::Dummy(ImVec2(1, DPI(10)));
		ImGui::Text("License info");
		ImGui::Separator();
		ImGui::TextWrapped("%s", OBV_LICENSE_TEXT_NONWRAPPED.c_str());

		if (ImGui::Button("Close") || keybindings.isPressed("CloseDialog")) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if (shown) {
		ImGui::OpenPopup("About");
	}
}

} // namespace Help
