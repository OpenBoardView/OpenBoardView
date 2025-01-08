#include "BoardView.h"

#include <string>
#include <vector>

#include "Program.h"

#include "imgui/imgui.h"

#include "GUI/DPI.h"
#include "GUI/widgets.h"

namespace Preferences {

Program::Program(KeyBindings &keybindings, Confparse &obvconfig, Config &config, BoardView &boardView)
	: keybindings(keybindings), obvconfig(obvconfig), config(config), boardView(boardView) {
}

void Program::menuItem() {
	if (ImGui::MenuItem("Program Preferences")) {
		// Make a copy to be able to restore if cancelled
		configCopy = config;
		shown = true;
	}
}

void Program::render() {
	bool p_open = true;
	auto &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), 0, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Program Preferences", &p_open, ImGuiWindowFlags_AlwaysAutoResize)) {
		shown = false;
		wasOpen = true;

		int t;

		t = config.windowX;
		RightAlignedText("Window width", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputInt("##windowX", &t)) {
			if (t > 400) config.windowX = t;
		}

		t = config.windowY;
		RightAlignedText("Window height", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputInt("##windowY", &t)) {
			if (t > 320) config.windowY = t;
		}

		std::vector<char> newFont(config.fontName.begin(), config.fontName.end()); // Copy string data + '\0' char
		if (newFont.size() < 256)                                          // reserve space for new font name
			newFont.resize(256, '\0');                                     // Max font name length is 255 characters
		RightAlignedText("Font name", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputText("##fontName", newFont.data(), newFont.size())) {
			config.fontName = newFont.data();
			boardView.reloadFonts = true;
		}

		t = config.fontSize;
		RightAlignedText("Font size", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputInt("##fontSize", &t)) {
			if (t < 8) {
				t = 8;
			} else if (t > 50) { // 50 is a value that hopefully should not break with too many fonts and 1024x1024 texture limits
				t = 50;
			}
			config.fontSize = t;
			boardView.reloadFonts = true;
		}

		t = config.dpi;
		RightAlignedText("Screen DPI", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputInt("##dpi", &t)) {
			if ((t > 25) && (t < 600)) {
				config.dpi = t;
				setDPI(t);
				boardView.reloadFonts = true;
			}
		}

#ifdef _WIN32
		ImGui::Separator();

		RightAlignedText("PDF software executable", DPI(250));
		ImGui::SameLine();
		ImGui::InputText("##pdfSoftwarePath", &config.pdfSoftwarePath;
		ImGui::SameLine();
		if (ImGui::Button("Browse##pdfSoftwarePath")) {
			auto path = show_file_picker();
			if (!path.empty()) {
				config.pdfSoftwarePath = path.string();
			}
		}
#endif

		ImGui::Separator();


		t = config.zoomFactor * 10;
		RightAlignedText("Zoom step", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputInt("##zoomStep", &t)) {
			config.zoomFactor = t / 10.0f;
		}

		RightAlignedText("Zoom modifier", DPI(250));
		ImGui::SameLine();
		ImGui::InputInt("##zoomModifier", &config.zoomModifier);

		RightAlignedText("Show info panel", DPI(250));
		ImGui::SameLine();
		ImGui::Checkbox("##showInfoPanel", &config.showInfoPanel);

		RightAlignedText("Info panel zoom", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputFloat("##partZoomScaleOutFactor", &config.partZoomScaleOutFactor)) {
			if (config.partZoomScaleOutFactor < 1.1) config.partZoomScaleOutFactor = 1.1;
		}

		RightAlignedText("Center/zoom search results", DPI(250));
		ImGui::SameLine();
		ImGui::Checkbox("##centerZoomSearchResults", &config.centerZoomSearchResults);

		RightAlignedText("Panning step", DPI(250));
		ImGui::SameLine();
		ImGui::InputInt("##panningStep", &config.panFactor);

		RightAlignedText("Pan modifier", DPI(250));
		ImGui::SameLine();
		ImGui::InputInt("##panModifier", &config.panModifier);

		RightAlignedText("Flip mode", DPI(250));
		ImGui::SameLine();
		ImGui::RadioButton("Viewport", &config.flipMode, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Mouse", &config.flipMode, 1);

		RightAlignedText("Show FPS", DPI(250));
		ImGui::SameLine();
		ImGui::Checkbox("##showFPS", &config.showFPS);

		ImGui::SameLine();
		RightAlignedText("Slow CPU", DPI(150));
		ImGui::SameLine();
		if (ImGui::Checkbox("##slowCPU", &config.slowCPU)) {
			style.AntiAliasedLines = !config.slowCPU;
			style.AntiAliasedFill  = !config.slowCPU;
		}

		ImGui::Separator();

		boardAppearance.render();

		ImGui::Separator();
		{
			char keybuf[1024];
			int i;
			ImGui::Text("FZ Key");
			ImGui::SameLine();
			for (i = 0; i < 44; i++) {
				sprintf(keybuf + (i * 12),
				        "0x%08lx%s",
				        (long unsigned int)config.FZKey[i],
				        (i != 43) ? ((i + 1) % 4 ? "  " : "\r\n")
				                  : ""); // yes, a nested inline-if-else. add \r\n after every 4 values, except if the last
			}
			if (ImGui::InputTextMultiline(
			        "##fzkey", keybuf, sizeof(keybuf), ImVec2(DPI(450), ImGui::GetTextLineHeight() * 12.5), 0, NULL, keybuf)) {

				// Strip the line breaks out.
				char *c = keybuf;
				while (*c) {
					if ((*c == '\r') || (*c == '\n')) *c = ' ';
					c++;
				}
				config.SetFZKey(keybuf);
			}
		}

		ImGui::Separator();
		{
			char caekeybuf[1024];
			int i;
			ImGui::Text("CAE Key");
			ImGui::SameLine();
			for (i = 0; i < 44; i++) {
				sprintf(caekeybuf + (i * 12),
				        "0x%08lx%s",
				        (long unsigned int)config.CAEKey[i],
				        (i != 43) ? ((i + 1) % 4 ? "  " : "\r\n")
				                  : ""); // yes, a nested inline-if-else. add \r\n after every 4 values, except if the last
			}
			if (ImGui::InputTextMultiline(
			        "##caekey", caekeybuf, sizeof(caekeybuf), ImVec2(DPI(450), ImGui::GetTextLineHeight() * 12.5), 0, NULL, caekeybuf)) {

				// Strip the line breaks out.
				char *c = caekeybuf;
				while (*c) {
					if ((*c == '\r') || (*c == '\n')) *c = ' ';
					c++;
				}
				config.SetCAEKey(caekeybuf);
			}
		}

		ImGui::Separator();

		if (ImGui::Button("Save")) {
			config.writeToConfig(obvconfig);
			ImGui::CloseCurrentPopup();
			wasOpen = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel") || this->keybindings.isPressed("CloseDialog")) {
			config = configCopy;
			setDPI(config.dpi);
			boardView.reloadFonts = true;
			ImGui::CloseCurrentPopup();
			wasOpen = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset")) {
			config = Config{};
		}

		ImGui::EndPopup();
	}

	if (!p_open) { // ImGui tells us the popup is closed
		if (wasOpen) {
			// We need to run this only once when the popup is closed with the popup title bar close button
			// If it was closed with our Save or Cancel button, we already did what we had to do and was_open is already false
			config = configCopy;
			setDPI(config.dpi);
			boardView.reloadFonts = true;
			wasOpen = false;
		}
	}

	if (shown) {
		ImGui::OpenPopup("Program Preferences");
	}
}

} // namespace Preferences
