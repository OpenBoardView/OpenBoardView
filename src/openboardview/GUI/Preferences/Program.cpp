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

		int t;

		t = obvconfig.ParseInt("windowX", 1100);
		RightAlignedText("Window width", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputInt("##windowX", &t)) {
			if (t > 400) obvconfig.WriteInt("windowX", t);
		}

		t = obvconfig.ParseInt("windowY", 700);
		RightAlignedText("Window height", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputInt("##windowY", &t)) {
			if (t > 320) obvconfig.WriteInt("windowY", t);
		}

		const char *oldFont = obvconfig.ParseStr("fontName", "");
		std::vector<char> newFont(oldFont, oldFont + strlen(oldFont) + 1); // Copy string data + '\0' char
		if (newFont.size() < 256)                                          // reserve space for new font name
			newFont.resize(256, '\0');                                     // Max font name length is 255 characters
		RightAlignedText("Font name", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputText("##fontName", newFont.data(), newFont.size())) {
			config.fontName = newFont.data();
			obvconfig.WriteStr("fontName", newFont.data());
			boardView.reloadFonts = true;
		}

		t = obvconfig.ParseInt("fontSize", 20);
		RightAlignedText("Font size", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputInt("##fontSize", &t)) {
			if (t < 8) {
				t = 8;
			} else if (t > 50) { // 50 is a value that hopefully should not break with too many fonts and 1024x1024 texture limits
				t = 50;
			}
			config.fontSize = t;
			obvconfig.WriteInt("fontSize", t);
			boardView.reloadFonts = true;
		}

		t = obvconfig.ParseInt("dpi", 100);
		RightAlignedText("Screen DPI", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputInt("##dpi", &t)) {
			if ((t > 25) && (t < 600)) {
				obvconfig.WriteInt("dpi", t);
				setDPI(t);
				boardView.reloadFonts = true;
			}
		}

#ifdef _WIN32
		ImGui::Separator();

		RightAlignedText("PDF software executable", DPI(250));
		ImGui::SameLine();
		static std::string pdfSoftwarePath = obvconfig.ParseStr("pdfSoftwarePath", "SumatraPDF.exe");;
		if (ImGui::InputText("##pdfSoftwarePath", &config.pdfSoftwarePath)) {
			obvconfig.WriteStr("pdfSoftwarePath", config.pdfSoftwarePath.c_str());
		}
		ImGui::SameLine();
		if (ImGui::Button("Browse##pdfSoftwarePath")) {
			auto path = show_file_picker();
			if (!path.empty()) {
				pdfSoftwarePath = path.string();
				obvconfig.WriteStr("pdfSoftwarePath", config.pdfSoftwarePath.c_str());
			}
		}
#endif

		ImGui::Separator();


		t = config.zoomFactor * 10;
		RightAlignedText("Zoom step", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputInt("##zoomStep", &t)) {
			config.zoomFactor = t / 10.0f;
			obvconfig.WriteFloat("zoomFactor", config.zoomFactor);
		}

		RightAlignedText("Zoom modifier", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputInt("##zoomModifier", &config.zoomModifier)) {
			obvconfig.WriteInt("zoomModifier", config.zoomModifier);
		}

		RightAlignedText("Show info panel", DPI(250));
		ImGui::SameLine();
		if (ImGui::Checkbox("##showInfoPanel", &config.showInfoPanel)) {
			obvconfig.WriteBool("showInfoPanel", config.showInfoPanel);
		}

		RightAlignedText("Info panel zoom", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputFloat("##partZoomScaleOutFactor", &config.partZoomScaleOutFactor)) {
			if (config.partZoomScaleOutFactor < 1.1) config.partZoomScaleOutFactor = 1.1;
			obvconfig.WriteFloat("partZoomScaleOutFactor", config.partZoomScaleOutFactor);
		}

		RightAlignedText("Center/zoom search results", DPI(250));
		ImGui::SameLine();
		if (ImGui::Checkbox("##centerZoomSearchResults", &config.centerZoomSearchResults)) {
			obvconfig.WriteBool("centerZoomSearchResults", config.centerZoomSearchResults);
		}

		RightAlignedText("Panning step", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputInt("##panningStep", &config.panFactor)) {
			obvconfig.WriteInt("panFactor", config.panFactor);
		}

		RightAlignedText("Pan modifier", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputInt("##panModifier", &config.panModifier)) {
			obvconfig.WriteInt("panModifier", config.panModifier);
		}

		RightAlignedText("Flip mode", DPI(250));
		ImGui::SameLine();
		{
			if (ImGui::RadioButton("Viewport", &config.flipMode, 0)) {
				obvconfig.WriteInt("flipMode", config.flipMode);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Mouse", &config.flipMode, 1)) {
				obvconfig.WriteInt("flipMode", config.flipMode);
			}
		}

		RightAlignedText("Show FPS", DPI(250));
		ImGui::SameLine();
		if (ImGui::Checkbox("##showFPS", &config.showFPS)) {
			obvconfig.WriteBool("showFPS", config.showFPS);
		}

		ImGui::SameLine();
		RightAlignedText("Slow CPU", DPI(150));
		ImGui::SameLine();
		if (ImGui::Checkbox("##slowCPU", &config.slowCPU)) {
			obvconfig.WriteBool("slowCPU", config.slowCPU);
			style.AntiAliasedLines = !config.slowCPU;
			style.AntiAliasedFill  = !config.slowCPU;
		}

		ImGui::Separator();

		RightAlignedText("Annotation flag size", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputInt("##annotationBoxSize", &config.annotationBoxSize)) {
			obvconfig.WriteInt("annotationBoxSize", config.annotationBoxSize);
		}

		RightAlignedText("Annotation flag offset", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputInt("##annotationBoxOffset", &config.annotationBoxOffset)) {
			obvconfig.WriteInt("annotationBoxOffset", config.annotationBoxOffset);
		}

		RightAlignedText("Pin-1/A1 count threshold", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputInt("##pinA1threshold", &config.pinA1threshold)) {
			if (config.pinA1threshold < 1) config.pinA1threshold = 1;
			obvconfig.WriteInt("pinA1threshold", config.pinA1threshold);
		}

		RightAlignedText("Pin select masks", DPI(250));
		ImGui::SameLine();
		if (ImGui::Checkbox("##pinSelectMasks", &config.pinSelectMasks)) {
			obvconfig.WriteBool("pinSelectMasks", config.pinSelectMasks);
		}

		RightAlignedText("Pin halo", DPI(250));
		ImGui::SameLine();
		if (ImGui::Checkbox("##pinHalo", &config.pinHalo)) {
			obvconfig.WriteBool("", config.pinHalo);
		}

		RightAlignedText("Halo diameter", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputFloat("##haloDiameter", &config.pinHaloDiameter)) {
			obvconfig.WriteFloat("pinHaloDiameter", config.pinHaloDiameter);
		}

		RightAlignedText("Halo thickness", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputFloat("##haloThickness", &config.pinHaloThickness)) {
			obvconfig.WriteFloat("pinHaloThickness", config.pinHaloThickness);
		}

		RightAlignedText("Show net web", DPI(250));
		ImGui::SameLine();
		if (ImGui::Checkbox("##showNetWeb", &config.showNetWeb)) {
			obvconfig.WriteBool("showNetWeb", config.showNetWeb);
		}

		RightAlignedText("Net web thickness", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputInt("##netWebThickness", &config.netWebThickness)) {
			obvconfig.WriteInt("netWebThickness", config.netWebThickness);
		}

		RightAlignedText("Fill parts", DPI(250));
		ImGui::SameLine();
		if (ImGui::Checkbox("##fillParts", &config.fillParts)) {
			obvconfig.WriteBool("fillParts", config.fillParts);
		}

		ImGui::SameLine();
		RightAlignedText("Fill board", DPI(150));
		ImGui::SameLine();
		if (ImGui::Checkbox("##boardFill", &config.boardFill)) {
			obvconfig.WriteBool("boardFill", config.boardFill);
		}

		RightAlignedText("Board fill spacing", DPI(250));
		ImGui::SameLine();
		if (ImGui::InputInt("##boardFillSpacing", &config.boardFillSpacing)) {
			obvconfig.WriteInt("boardFillSpacing", config.boardFillSpacing);
		}

		RightAlignedText("Show parts name", DPI(250));
		ImGui::SameLine();
		if (ImGui::Checkbox("##showPartName", &config.showPartName)) {
			obvconfig.WriteBool("showPartName", config.showPartName);
		}

		ImGui::SameLine();
		RightAlignedText("Show pins name", DPI(150));
		ImGui::SameLine();
		if (ImGui::Checkbox("##showPinName", &config.showPinName)) {
			obvconfig.WriteBool("showPinName", config.showPinName);
		}

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
				obvconfig.WriteStr("FZKey", keybuf);
				config.SetFZKey(keybuf);
			}
		}

		ImGui::Separator();

		if (ImGui::Button("Done") || keybindings.isPressed("CloseDialog")) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if (shown) {
		ImGui::OpenPopup("Program Preferences");
	}
}

} // namespace Preferences
