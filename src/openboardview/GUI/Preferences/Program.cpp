#include "BoardView.h"

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
		RightAlignedText("Window Width", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##windowX", &t)) {
			if (t > 400) obvconfig.WriteInt("windowX", t);
		}

		t = obvconfig.ParseInt("windowY", 700);
		RightAlignedText("Window Height", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##windowY", &t)) {
			if (t > 320) obvconfig.WriteInt("windowY", t);
		}

		const char *oldFont = obvconfig.ParseStr("fontName", "");
		std::vector<char> newFont(oldFont, oldFont + strlen(oldFont) + 1); // Copy string data + '\0' char
		if (newFont.size() < 256)                                          // reserve space for new font name
			newFont.resize(256, '\0');                                     // Max font name length is 255 characters
		RightAlignedText("Font name", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputText("##fontName", newFont.data(), newFont.size())) {
			config.fontName = newFont.data();
			obvconfig.WriteStr("fontName", newFont.data());
			boardView.reloadFonts = true;
		}

		t = obvconfig.ParseInt("fontSize", 20);
		RightAlignedText("Font size", DPI(200));
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
		RightAlignedText("Screen DPI", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##dpi", &t)) {
			if ((t > 25) && (t < 600)) {
				obvconfig.WriteInt("dpi", t);
				setDPI(t);
				boardView.reloadFonts = true;
			}
		}

		ImGui::Separator();

		RightAlignedText("Board fill spacing", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##boardFillSpacing", &config.boardFillSpacing)) {
			obvconfig.WriteInt("boardFillSpacing", config.boardFillSpacing);
		}

		t = config.zoomFactor * 10;
		RightAlignedText("Zoom step", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##zoomStep", &t)) {
			config.zoomFactor = t / 10.0f;
			obvconfig.WriteFloat("zoomFactor", config.zoomFactor);
		}

		RightAlignedText("Zoom modifier", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##zoomModifier", &config.zoomModifier)) {
			obvconfig.WriteInt("zoomModifier", config.zoomModifier);
		}

		RightAlignedText("Panning step", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##panningStep", &config.panFactor)) {
			obvconfig.WriteInt("panFactor", config.panFactor);
		}

		RightAlignedText("Pan modifier", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##panModifier", &config.panModifier)) {
			obvconfig.WriteInt("panModifier", config.panModifier);
		}

		ImGui::Dummy(ImVec2(1, DPI(5)));
		RightAlignedText("Flip Mode", DPI(200));
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
		ImGui::Dummy(ImVec2(1, DPI(5)));

		RightAlignedText("Annotation flag size", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##annotationBoxSize", &config.annotationBoxSize)) {
			obvconfig.WriteInt("annotationBoxSize", config.annotationBoxSize);
		}

		RightAlignedText("Annotation flag offset", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##annotationBoxOffset", &config.annotationBoxOffset)) {
			obvconfig.WriteInt("annotationBoxOffset", config.annotationBoxOffset);
		}
		RightAlignedText("Net web thickness", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##netWebThickness", &config.netWebThickness)) {
			obvconfig.WriteInt("netWebThickness", config.netWebThickness);
		}
		ImGui::Separator();

		RightAlignedText("Pin-1/A1 count threshold", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##pinA1threshold", &config.pinA1threshold)) {
			if (config.pinA1threshold < 1) config.pinA1threshold = 1;
			obvconfig.WriteInt("pinA1threshold", config.pinA1threshold);
		}

		if (ImGui::Checkbox("Pin select masks", &config.pinSelectMasks)) {
			obvconfig.WriteBool("pinSelectMasks", config.pinSelectMasks);
		}

		if (ImGui::Checkbox("Pin Halo", &config.pinHalo)) {
			obvconfig.WriteBool("pinHalo", config.pinHalo);
		}
		RightAlignedText("Halo diameter", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputFloat("##haloDiameter", &config.pinHaloDiameter)) {
			obvconfig.WriteFloat("pinHaloDiameter", config.pinHaloDiameter);
		}

		RightAlignedText("Halo thickness", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputFloat("##haloThickness", &config.pinHaloThickness)) {
			obvconfig.WriteFloat("pinHaloThickness", config.pinHaloThickness);
		}

		RightAlignedText("Info Panel Zoom", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputFloat("##partZoomScaleOutFactor", &config.partZoomScaleOutFactor)) {
			if (config.partZoomScaleOutFactor < 1.1) config.partZoomScaleOutFactor = 1.1;
			obvconfig.WriteFloat("partZoomScaleOutFactor", config.partZoomScaleOutFactor);
		}

		if (ImGui::Checkbox("Center/Zoom Search Results", &config.centerZoomSearchResults)) {
			obvconfig.WriteBool("centerZoomSearchResults", config.centerZoomSearchResults);
		}

		if (ImGui::Checkbox("Show Info Panel", &config.showInfoPanel)) {
			obvconfig.WriteBool("showInfoPanel", config.showInfoPanel);
		}

		if (ImGui::Checkbox("Show net web", &config.showNetWeb)) {
			obvconfig.WriteBool("showNetWeb", config.showNetWeb);
		}

		if (ImGui::Checkbox("slowCPU", &config.slowCPU)) {
			obvconfig.WriteBool("slowCPU", config.slowCPU);
			style.AntiAliasedLines = !config.slowCPU;
			style.AntiAliasedFill  = !config.slowCPU;
		}

		ImGui::SameLine();
		if (ImGui::Checkbox("Show FPS", &config.showFPS)) {
			obvconfig.WriteBool("showFPS", config.showFPS);
		}

		if (ImGui::Checkbox("Fill Parts", &config.fillParts)) {
			obvconfig.WriteBool("fillParts", config.fillParts);
		}

		ImGui::SameLine();
		if (ImGui::Checkbox("Fill Board", &config.boardFill)) {
			obvconfig.WriteBool("boardFill", config.boardFill);
		}

		if (ImGui::Checkbox("Show part name", &config.showPartName)) {
			obvconfig.WriteBool("showPartName", config.showPartName);
		}

		ImGui::SameLine();
		if (ImGui::Checkbox("Show pin name", &config.showPinName)) {
			obvconfig.WriteBool("showPinName", config.showPinName);
		}

#ifdef _WIN32
		RightAlignedText("PDF software executable", DPI(200));
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
