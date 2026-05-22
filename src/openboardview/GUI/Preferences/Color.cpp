#include "Color.h"

#include "imgui/imgui.h"

#include "GUI/DPI.h"
#include "GUI/widgets.h"

namespace Preferences {


Color::Color(KeyBindings &keyBindings, Confparse &obvconfig, ColorScheme &colorScheme)
	: keyBindings(keyBindings), obvconfig(obvconfig), colorScheme(colorScheme) {
}

void Color::ColorPreferencesItem(
    const char *label, int label_width, const char *butlabel, const char *conflabel, int var_width, uint32_t *c) {
	std::string desc_id = "##ColorButton" + std::string(label);
	char buf[20];
	snprintf(buf, sizeof(buf), "%08X", ColorScheme::byte4swap(*c));
	RightAlignedText(label, label_width);
	ImGui::SameLine();
	ImGui::ColorButton(desc_id.c_str(), ImColor(*c));
	ImGui::SameLine();
	ImGui::PushItemWidth(var_width);
	if (ImGui::InputText(
	        butlabel, buf, sizeof(buf), ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsHexadecimal, nullptr, buf)) {
		*c = ColorScheme::byte4swap(strtoll(buf, NULL, 16));
		snprintf(buf, sizeof(buf), "0x%08x", ColorScheme::byte4swap(*c));
		obvconfig.WriteStr(conflabel, buf);
	}
	ImGui::PopItemWidth();
}

void Color::menuItem() {
	if (ImGui::MenuItem("Colour Preferences")) {
		shown = true;
	}
}

bool Color::render() {
	bool m_needsRedraw = false;
	bool p_open = true;

	//	ImGui::PushStyleColor(ImGuiCol_PopupBg, 0xffe0e0e0);
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), 0, ImVec2(0.5f, 0.5f));;
	if (ImGui::BeginPopupModal("Colour Preferences", &p_open, ImGuiWindowFlags_AlwaysAutoResize)) {
		shown = false;

		ImGui::Dummy(ImVec2(1, DPI(5)));
		RightAlignedText("Base theme", DPI(200));
		ImGui::SameLine();
		{
			int tc        = 0;
			const char *v = obvconfig.ParseStr("colorTheme", "default");
			if (strcmp(v, "dark") == 0) {
				tc = 1;
			}
			if (ImGui::RadioButton("Light", &tc, 0)) {
				obvconfig.WriteStr("colorTheme", "light");
				colorScheme.ThemeSetStyle("light");
				colorScheme.writeToConfig(obvconfig);
				m_needsRedraw = true;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Dark", &tc, 1)) {
				obvconfig.WriteStr("colorTheme", "dark");
				colorScheme.ThemeSetStyle("dark");
				colorScheme.writeToConfig(obvconfig);
				m_needsRedraw = true;
			}
		}
		ImGui::Dummy(ImVec2(1, DPI(5)));

		ColorPreferencesItem("Background", DPI(200), "##Background", "backgroundColor", DPI(150), &colorScheme.backgroundColor);
		ColorPreferencesItem("Board fill", DPI(200), "##Boardfill", "boardFillColor", DPI(150), &colorScheme.boardFillColor);
		ColorPreferencesItem(
		    "Board outline", DPI(200), "##BoardOutline", "boardOutlineColor", DPI(150), &colorScheme.boardOutlineColor);

		ImGui::Dummy(ImVec2(1, DPI(10)));
		ImGui::Text("Parts");
		ImGui::Separator();
		ColorPreferencesItem("Outline", DPI(200), "##PartOutline", "partOutlineColor", DPI(150), &colorScheme.partOutlineColor);
		ColorPreferencesItem("Hull", DPI(200), "##PartHull", "partHullColor", DPI(150), &colorScheme.partHullColor);
		ColorPreferencesItem("Fill", DPI(200), "##PartFill", "partFillColor", DPI(150), &colorScheme.partFillColor);
		ColorPreferencesItem("Text", DPI(200), "##PartText", "partTextColor", DPI(150), &colorScheme.partTextColor);
		ColorPreferencesItem(
		    "Selected", DPI(200), "##PartSelected", "partHighlightedColor", DPI(150), &colorScheme.partHighlightedColor);
		ColorPreferencesItem("Fill (selected)",
		                     DPI(200),
		                     "##PartFillSelected",
		                     "partHighlightedFillColor",
		                     DPI(150),
		                     &colorScheme.partHighlightedFillColor);
		ColorPreferencesItem("Text (selected)", DPI(200), "##PartHighlightedText", "partHighlightedTextColor", DPI(150), &colorScheme.partHighlightedTextColor);
		ColorPreferencesItem("Text background (selected)",
		                     DPI(200),
		                     "##PartHighlightedTextBackground",
		                     "partHighlightedTextBackgroundColor",
		                     DPI(150),
		                     &colorScheme.partHighlightedTextBackgroundColor);

		ImGui::Dummy(ImVec2(1, DPI(10)));
		ImGui::Text("Pins");
		ImGui::Separator();
		ColorPreferencesItem("Default", DPI(200), "##PinDefault", "pinDefaultColor", DPI(150), &colorScheme.pinDefaultColor);
		ColorPreferencesItem(
		    "Default text", DPI(200), "##PinDefaultText", "pinDefaultTextColor", DPI(150), &colorScheme.pinDefaultTextColor);
		ColorPreferencesItem(
		    "Text background", DPI(200), "##PinTextBackground", "pinTextBackgroundColor", DPI(150), &colorScheme.pinTextBackgroundColor);
		ColorPreferencesItem("Ground", DPI(200), "##PinGround", "pinGroundColor", DPI(150), &colorScheme.pinGroundColor);
		ColorPreferencesItem("NC", DPI(200), "##PinNC", "pinNotConnectedColor", DPI(150), &colorScheme.pinNotConnectedColor);
		ColorPreferencesItem("Test pad", DPI(200), "##PinTP", "pinTestPadColor", DPI(150), &colorScheme.pinTestPadColor);
		ColorPreferencesItem("Test pad fill", DPI(200), "##PinTPF", "pinTestPadFillColor", DPI(150), &colorScheme.pinTestPadFillColor);
		ColorPreferencesItem("A1/1 Pad", DPI(200), "##PinA1", "pinA1PadColor", DPI(150), &colorScheme.pinA1PadColor);

		ColorPreferencesItem("Selected", DPI(200), "##PinSelectedColor", "pinSelectedColor", DPI(150), &colorScheme.pinSelectedColor);
		ColorPreferencesItem(
		    "Selected fill", DPI(200), "##PinSelectedFillColor", "pinSelectedFillColor", DPI(150), &colorScheme.pinSelectedFillColor);
		ColorPreferencesItem(
		    "Selected text", DPI(200), "##PinSelectedTextColor", "pinSelectedTextColor", DPI(150), &colorScheme.pinSelectedTextColor);

		ColorPreferencesItem("Same Net", DPI(200), "##PinSameNetColor", "pinSameNetColor", DPI(150), &colorScheme.pinSameNetColor);
		ColorPreferencesItem(
		    "SameNet fill", DPI(200), "##PinSameNetFillColor", "pinSameNetFillColor", DPI(150), &colorScheme.pinSameNetFillColor);
		ColorPreferencesItem(
		    "SameNet text", DPI(200), "##PinSameNetTextColor", "pinSameNetTextColor", DPI(150), &colorScheme.pinSameNetTextColor);

		ColorPreferencesItem("Halo", DPI(200), "##PinHalo", "pinHaloColor", DPI(150), &colorScheme.pinHaloColor);
		ColorPreferencesItem("Net web strands", DPI(200), "##NetWebStrands", "pinNetWebColor", DPI(150), &colorScheme.pinNetWebColor);
		ColorPreferencesItem(
		    "Net web (otherside)", DPI(200), "##NetWebOSStrands", "pinNetWebOSColor", DPI(150), &colorScheme.pinNetWebOSColor);

		ImGui::Dummy(ImVec2(1, DPI(10)));
		ImGui::Text("Annotations");
		ImGui::Separator();
		ColorPreferencesItem("Box", DPI(200), "##AnnBox", "annotationBoxColor", DPI(150), &colorScheme.annotationBoxColor);
		ColorPreferencesItem("Stalk", DPI(200), "##AnnStalk", "annotationStalkColor", DPI(150), &colorScheme.annotationStalkColor);
		ColorPreferencesItem(
		    "Popup text", DPI(200), "##AnnPopupText", "annotationPopupTextColor", DPI(150), &colorScheme.annotationPopupTextColor);
		ColorPreferencesItem("Popup background",
		                     DPI(200),
		                     "##AnnPopupBackground",
		                     "annotationPopupBackgroundColor",
		                     DPI(150),
		                     &colorScheme.annotationPopupBackgroundColor);

		ImGui::Dummy(ImVec2(1, DPI(10)));
		ImGui::Text("Masks");
		ImGui::Separator();
		ColorPreferencesItem("Pins", DPI(200), "##MaskPins", "selectedMaskPins", DPI(150), &colorScheme.selectedMaskPins);
		ColorPreferencesItem("Parts", DPI(200), "##MaskParts", "selectedMaskParts", DPI(150), &colorScheme.selectedMaskParts);
		ColorPreferencesItem("Outline", DPI(200), "##MaskOutline", "selectedMaskOutline", DPI(150), &colorScheme.selectedMaskOutline);
		ColorPreferencesItem("Boost (Pins)", DPI(200), "##BoostPins", "orMaskPins", DPI(150), &colorScheme.orMaskPins);
		ColorPreferencesItem("Boost (Parts)", DPI(200), "##BoostParts", "orMaskParts", DPI(150), &colorScheme.orMaskParts);
		ColorPreferencesItem("Boost (Outline)", DPI(200), "##BoostOutline", "orMaskOutline", DPI(150), &colorScheme.orMaskOutline);

		ImGui::Separator();

		if (ImGui::Button("Done") || keyBindings.isPressed("CloseDialog")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (shown) {
		ImGui::OpenPopup("Colour Preferences");
	}
	//	ImGui::PopStyleColor();
	//
	return m_needsRedraw;
}

} // namespace Preferences
