#include "ColorScheme.h"

#include <cstdint>

#include "imgui/imgui.h"

uint32_t ColorScheme::byte4swap(uint32_t x) {
	/*
	 * used to convert RGBA -> ABGR etc
	 */
	return (((x & 0x000000ff) << 24) | ((x & 0x0000ff00) << 8) | ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24));
}

void ColorScheme::ThemeSetStyle(const char *name) {
	ImGuiStyle &style = ImGui::GetStyle();

	// non color-related settings
	style.WindowBorderSize = 0.0f;

	if (strcmp(name, "dark") == 0) {
		style.Colors[ImGuiCol_Text]          = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled]  = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
		style.Colors[ImGuiCol_WindowBg]      = ImVec4(0.00f, 0.00f, 0.00f, 0.70f);
		style.Colors[ImGuiCol_ChildBg]       = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_PopupBg]       = ImVec4(0.05f, 0.05f, 0.10f, 0.90f);
		style.Colors[ImGuiCol_Border]        = ImVec4(0.70f, 0.70f, 0.70f, 0.65f);
		style.Colors[ImGuiCol_BorderShadow]  = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_FrameBg] =
		    ImVec4(0.30f, 0.30f, 0.30f, 1.0f); // Background of checkbox, radio button, plot, slider, text input
		style.Colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.90f, 0.80f, 0.80f, 0.40f);
		style.Colors[ImGuiCol_FrameBgActive]        = ImVec4(0.90f, 0.65f, 0.65f, 0.45f);
		style.Colors[ImGuiCol_TitleBg]              = ImVec4(0.27f, 0.27f, 0.54f, 0.83f);
		style.Colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
		style.Colors[ImGuiCol_TitleBgActive]        = ImVec4(0.32f, 0.32f, 0.63f, 0.87f);
		style.Colors[ImGuiCol_MenuBarBg]            = ImVec4(0.40f, 0.40f, 0.55f, 0.80f);
		style.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
		style.Colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.40f, 0.40f, 0.80f, 0.30f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.80f, 0.40f);
		style.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.80f, 0.50f, 0.50f, 0.40f);
		style.Colors[ImGuiCol_CheckMark]            = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
		style.Colors[ImGuiCol_SliderGrab]           = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
		style.Colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
		style.Colors[ImGuiCol_Button]               = ImVec4(0.67f, 0.40f, 0.40f, 0.60f);
		style.Colors[ImGuiCol_ButtonHovered]        = ImVec4(0.67f, 0.40f, 0.40f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive]         = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
		style.Colors[ImGuiCol_Header]               = ImVec4(0.40f, 0.40f, 0.90f, 0.45f);
		style.Colors[ImGuiCol_HeaderHovered]        = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
		style.Colors[ImGuiCol_HeaderActive]         = ImVec4(0.53f, 0.53f, 0.87f, 0.80f);
		style.Colors[ImGuiCol_Separator]            = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		style.Colors[ImGuiCol_SeparatorHovered]     = ImVec4(0.70f, 0.60f, 0.60f, 1.00f);
		style.Colors[ImGuiCol_SeparatorActive]      = ImVec4(0.90f, 0.70f, 0.70f, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip]           = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
		style.Colors[ImGuiCol_ResizeGripHovered]    = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
		style.Colors[ImGuiCol_ResizeGripActive]     = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
		style.Colors[ImGuiCol_PlotLines]            = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_PlotLinesHovered]     = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram]        = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg]       = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
		style.Colors[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
		style.Colors[ImGuiCol_TableRowBg]           = ImVec4(0.05f, 0.05f, 0.10f, 0.90f);
		style.Colors[ImGuiCol_TableRowBgAlt]        = ImVec4(0.10f, 0.10f, 0.20f, 0.90f);

		this->backgroundColor          = byte4swap(0x000000ff);
		this->boardFillColor           = byte4swap(0x2a2a2aff);
		this->boardOutlineColor        = byte4swap(0xcc2222ff);
		this->partHullColor            = byte4swap(0x80808080);
		this->partOutlineColor         = byte4swap(0x999999ff);
		this->partFillColor            = byte4swap(0x111111ff);
		this->partTextColor            = byte4swap(0x80808080);
		this->partHighlightedColor     = byte4swap(0xffffffff);
		this->partHighlightedFillColor = byte4swap(0x333333ff);
		this->partHighlightedTextColor            = byte4swap(0x000000ff);
		this->partHighlightedTextBackgroundColor  = byte4swap(0xcccc22ff);
		this->pinDefaultColor          = byte4swap(0x4040ffff);
		this->pinDefaultTextColor      = byte4swap(0xccccccff);
		this->pinTextBackgroundColor   = byte4swap(0xffffff80);
		this->pinGroundColor           = byte4swap(0x0300C3ff);
		this->pinNotConnectedColor     = byte4swap(0xaaaaaaff);
		this->pinTestPadColor          = byte4swap(0x888888ff);
		this->pinTestPadFillColor      = byte4swap(0x6c5b1fff);
		this->pinA1PadColor            = byte4swap(0xdd0000ff);

		this->pinSelectedColor     = byte4swap(0x00ff00ff);
		this->pinSelectedFillColor = byte4swap(0x8888ffff);
		this->pinSelectedTextColor = byte4swap(0xffffffff);

		this->pinSameNetColor     = byte4swap(0x0000ffff);
		this->pinSameNetFillColor = byte4swap(0x9999ffff);
		this->pinSameNetTextColor = byte4swap(0x111111ff);

		this->pinHaloColor     = byte4swap(0xffffff88);
		this->pinNetWebColor   = byte4swap(0xff888888);
		this->pinNetWebOSColor = byte4swap(0x8888ff88);

		this->annotationPartAliasColor       = byte4swap(0xffff00ff);
		this->annotationBoxColor             = byte4swap(0xcccc88ff);
		this->annotationStalkColor           = byte4swap(0xaaaaaaff);
		this->annotationPopupBackgroundColor = byte4swap(0x888888ff);
		this->annotationPopupTextColor       = byte4swap(0xffffffff);

		this->selectedMaskPins    = byte4swap(0xffffffff);
		this->selectedMaskParts   = byte4swap(0xffffffff);
		this->selectedMaskOutline = byte4swap(0xffffffff);

		this->orMaskPins    = byte4swap(0x0);
		this->orMaskParts   = byte4swap(0x0);
		this->orMaskOutline = byte4swap(0x0);

	} else {
		// light theme default
		style.Colors[ImGuiCol_Text]                 = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled]         = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
		style.Colors[ImGuiCol_PopupBg]              = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
		style.Colors[ImGuiCol_WindowBg]             = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
		style.Colors[ImGuiCol_ChildBg]              = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_Border]               = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
		style.Colors[ImGuiCol_BorderShadow]         = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
		style.Colors[ImGuiCol_FrameBg]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		style.Colors[ImGuiCol_FrameBgActive]        = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		style.Colors[ImGuiCol_TitleBg]              = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
		style.Colors[ImGuiCol_TitleBgActive]        = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
		style.Colors[ImGuiCol_MenuBarBg]            = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
		style.Colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
		style.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
		style.Colors[ImGuiCol_CheckMark]            = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_SliderGrab]           = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
		style.Colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_Button]               = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		style.Colors[ImGuiCol_ButtonHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive]         = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_Header]               = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
		style.Colors[ImGuiCol_HeaderHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		style.Colors[ImGuiCol_HeaderActive]         = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_Separator]            = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
		style.Colors[ImGuiCol_SeparatorHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
		style.Colors[ImGuiCol_SeparatorActive]      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip]           = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
		style.Colors[ImGuiCol_ResizeGripHovered]    = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		style.Colors[ImGuiCol_ResizeGripActive]     = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		style.Colors[ImGuiCol_PlotLines]            = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
		style.Colors[ImGuiCol_PlotLinesHovered]     = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram]        = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg]       = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		style.Colors[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
		style.Colors[ImGuiCol_TableRowBg]           = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
		style.Colors[ImGuiCol_TableRowBgAlt]        = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);

		this->backgroundColor          = byte4swap(0xffffffff);
		this->boardFillColor           = byte4swap(0xddddddff);
		this->partHullColor            = byte4swap(0x80808080);
		this->partOutlineColor         = byte4swap(0x444444ff);
		this->partFillColor            = byte4swap(0xffffff77);
		this->partTextColor            = byte4swap(0x80808080);
		this->partHighlightedColor     = byte4swap(0xff0000ff);
		this->partHighlightedFillColor = byte4swap(0xf0f0f0ff);
		this->partHighlightedTextColor            = byte4swap(0xff3030ff);
		this->partHighlightedTextBackgroundColor  = byte4swap(0xffff00ff);
		this->boardOutlineColor        = byte4swap(0x444444ff);
		this->pinDefaultColor          = byte4swap(0x22aa33ff);
		this->pinDefaultTextColor      = byte4swap(0x666688ff);
		this->pinTextBackgroundColor   = byte4swap(0xffffff80);
		this->pinGroundColor           = byte4swap(0x2222aaff);
		this->pinNotConnectedColor     = byte4swap(0xaaaaaaff);
		this->pinTestPadColor          = byte4swap(0x888888ff);
		this->pinTestPadFillColor      = byte4swap(0xd6c68dff);
		this->pinA1PadColor            = byte4swap(0xdd0000ff);

		this->pinSelectedColor     = byte4swap(0x00000000);
		this->pinSelectedFillColor = byte4swap(0x8888ffff);
		this->pinSelectedTextColor = byte4swap(0xffffffff);

		this->pinSameNetColor     = byte4swap(0x888888ff);
		this->pinSameNetFillColor = byte4swap(0x9999ffff);
		this->pinSameNetTextColor = byte4swap(0x111111ff);

		this->pinHaloColor     = byte4swap(0x22FF2288);
		this->pinNetWebColor   = byte4swap(0xff0000aa);
		this->pinNetWebOSColor = byte4swap(0x0000ff33);

		this->annotationPartAliasColor       = byte4swap(0xffff00ff);
		this->annotationBoxColor             = byte4swap(0xff0000aa);
		this->annotationStalkColor           = byte4swap(0x000000ff);
		this->annotationPopupBackgroundColor = byte4swap(0xeeeeeeff);
		this->annotationPopupTextColor       = byte4swap(0x000000ff);

		this->selectedMaskPins    = byte4swap(0xffffffff);
		this->selectedMaskParts   = byte4swap(0xffffffff);
		this->selectedMaskOutline = byte4swap(0xffffffff);

		this->orMaskPins    = byte4swap(0x0);
		this->orMaskParts   = byte4swap(0x0);
		this->orMaskOutline = byte4swap(0x0);
	}
}

void ColorScheme::readFromConfig(Confparse &obvconfig) {
	const char *v = obvconfig.ParseStr("colorTheme", "light");
	ThemeSetStyle(v);

	/*
	 * Colours in ImGui can be represented as a 4-byte packed uint32_t as ABGR
	 * but most humans are more accustomed to RBGA, so for the sake of readability
	 * we use the human-readable version but swap the ordering around when
	 * it comes to assigning the actual colour to ImGui.
	 */

	/*
	 * XRayBlue theme
	 */

	this->backgroundColor = byte4swap(obvconfig.ParseHex("backgroundColor", byte4swap(this->backgroundColor)));
	this->boardFillColor  = byte4swap(obvconfig.ParseHex("boardFillColor", byte4swap(this->boardFillColor)));

	this->partHullColor        = byte4swap(obvconfig.ParseHex("partHullColor", byte4swap(this->partHullColor)));
	this->partOutlineColor     = byte4swap(obvconfig.ParseHex("partOutlineColor", byte4swap(this->partOutlineColor)));
	this->partFillColor        = byte4swap(obvconfig.ParseHex("partFillColor", byte4swap(this->partFillColor)));
	this->partTextColor = byte4swap(obvconfig.ParseHex("partTextColor", byte4swap(this->partTextColor)));
	this->partHighlightedColor = byte4swap(obvconfig.ParseHex("partHighlightedColor", byte4swap(this->partHighlightedColor)));
	this->partHighlightedFillColor =
	    byte4swap(obvconfig.ParseHex("partHighlightedFillColor", byte4swap(this->partHighlightedFillColor)));
	this->partHighlightedTextColor = byte4swap(obvconfig.ParseHex("partHighlightedTextColor", byte4swap(this->partHighlightedTextColor)));
	this->partHighlightedTextBackgroundColor =
	    byte4swap(obvconfig.ParseHex("partHighlightedTextBackgroundColor", byte4swap(this->partHighlightedTextBackgroundColor)));
	this->boardOutlineColor    = byte4swap(obvconfig.ParseHex("boardOutlineColor", byte4swap(this->boardOutlineColor)));
	this->pinDefaultColor      = byte4swap(obvconfig.ParseHex("pinDefaultColor", byte4swap(this->pinDefaultColor)));
	this->pinDefaultTextColor  = byte4swap(obvconfig.ParseHex("pinDefaultTextColor", byte4swap(this->pinDefaultTextColor)));
	this->pinTextBackgroundColor  = byte4swap(obvconfig.ParseHex("pinTextBackgroundColor", byte4swap(this->pinTextBackgroundColor)));
	this->pinGroundColor       = byte4swap(obvconfig.ParseHex("pinGroundColor", byte4swap(this->pinGroundColor)));
	this->pinNotConnectedColor = byte4swap(obvconfig.ParseHex("pinNotConnectedColor", byte4swap(this->pinNotConnectedColor)));
	this->pinTestPadColor      = byte4swap(obvconfig.ParseHex("pinTestPadColor", byte4swap(this->pinTestPadColor)));
	this->pinTestPadFillColor  = byte4swap(obvconfig.ParseHex("pinTestPadFillColor", byte4swap(this->pinTestPadFillColor)));
	this->pinA1PadColor        = byte4swap(obvconfig.ParseHex("pinA1PadColor", byte4swap(this->pinA1PadColor)));

	this->pinSelectedTextColor = byte4swap(obvconfig.ParseHex("pinSelectedTextColor", byte4swap(this->pinSelectedTextColor)));
	this->pinSelectedFillColor = byte4swap(obvconfig.ParseHex("pinSelectedFillColor", byte4swap(this->pinSelectedFillColor)));
	this->pinSelectedColor     = byte4swap(obvconfig.ParseHex("pinSelectedColor", byte4swap(this->pinSelectedColor)));

	this->pinSameNetTextColor = byte4swap(obvconfig.ParseHex("pinSameNetTextColor", byte4swap(this->pinSameNetTextColor)));
	this->pinSameNetFillColor = byte4swap(obvconfig.ParseHex("pinSameNetFillColor", byte4swap(this->pinSameNetFillColor)));
	this->pinSameNetColor     = byte4swap(obvconfig.ParseHex("pinSameNetColor", byte4swap(this->pinSameNetColor)));

	this->pinHaloColor   = byte4swap(obvconfig.ParseHex("pinHaloColor", byte4swap(this->pinHaloColor)));
	this->pinNetWebColor = byte4swap(obvconfig.ParseHex("pinNetWebColor", byte4swap(this->pinNetWebColor)));

	this->annotationPartAliasColor =
	    byte4swap(obvconfig.ParseHex("annotationPartAliasColor", byte4swap(this->annotationPartAliasColor)));
	this->annotationBoxColor   = byte4swap(obvconfig.ParseHex("annotationBoxColor", byte4swap(this->annotationBoxColor)));
	this->annotationStalkColor = byte4swap(obvconfig.ParseHex("annotationStalkColor", byte4swap(this->annotationStalkColor)));
	this->annotationPopupBackgroundColor =
	    byte4swap(obvconfig.ParseHex("annotationPopupBackgroundColor", byte4swap(this->annotationPopupBackgroundColor)));
	this->annotationPopupTextColor =
	    byte4swap(obvconfig.ParseHex("annotationPopupTextColor", byte4swap(this->annotationPopupTextColor)));

	this->selectedMaskPins    = byte4swap(obvconfig.ParseHex("selectedMaskPins", byte4swap(this->selectedMaskPins)));
	this->selectedMaskParts   = byte4swap(obvconfig.ParseHex("selectedMaskParts", byte4swap(this->selectedMaskParts)));
	this->selectedMaskOutline = byte4swap(obvconfig.ParseHex("selectedMaskOutline", byte4swap(this->selectedMaskOutline)));

	this->orMaskPins    = byte4swap(obvconfig.ParseHex("orMaskPins", byte4swap(this->orMaskPins)));
	this->orMaskParts   = byte4swap(obvconfig.ParseHex("orMaskParts", byte4swap(this->orMaskParts)));
	this->orMaskOutline = byte4swap(obvconfig.ParseHex("orMaskOutline", byte4swap(this->orMaskOutline)));
}

void ColorScheme::writeToConfig(Confparse &obvconfig) {
	obvconfig.WriteHex("backgroundColor", byte4swap(this->backgroundColor));
	obvconfig.WriteHex("boardFillColor", byte4swap(this->boardFillColor));
	obvconfig.WriteHex("boardOutlineColor", byte4swap(this->boardOutlineColor));
	obvconfig.WriteHex("partOutlineColor", byte4swap(this->partOutlineColor));
	obvconfig.WriteHex("partHullColor", byte4swap(this->partHullColor));
	obvconfig.WriteHex("partFillColor", byte4swap(this->partFillColor));
	obvconfig.WriteHex("partTextColor", byte4swap(this->partTextColor));
	obvconfig.WriteHex("partHighlightedColor", byte4swap(this->partHighlightedColor));
	obvconfig.WriteHex("partHighlightedFillColor", byte4swap(this->partHighlightedFillColor));
	obvconfig.WriteHex("partHighlightedTextColor", byte4swap(this->partHighlightedTextColor));
	obvconfig.WriteHex("partHighlightedTextBackgroundColor", byte4swap(this->partHighlightedTextBackgroundColor));
	obvconfig.WriteHex("pinDefaultColor", byte4swap(this->pinDefaultColor));
	obvconfig.WriteHex("pinDefaultTextColor", byte4swap(this->pinDefaultTextColor));
	obvconfig.WriteHex("pinTextBackgroundColor", byte4swap(this->pinTextBackgroundColor));
	obvconfig.WriteHex("pinGroundColor", byte4swap(this->pinGroundColor));
	obvconfig.WriteHex("pinNotConnectedColor", byte4swap(this->pinNotConnectedColor));
	obvconfig.WriteHex("pinTestPadColor", byte4swap(this->pinTestPadColor));
	obvconfig.WriteHex("pinTestPadFillColor", byte4swap(this->pinTestPadFillColor));
	obvconfig.WriteHex("pinA1PadColor", byte4swap(this->pinA1PadColor));
	obvconfig.WriteHex("pinSelectedColor", byte4swap(this->pinSelectedColor));
	obvconfig.WriteHex("pinSelectedTextColor", byte4swap(this->pinSelectedTextColor));
	obvconfig.WriteHex("pinSelectedFillColor", byte4swap(this->pinSelectedFillColor));
	obvconfig.WriteHex("pinSameNetColor", byte4swap(this->pinSameNetColor));
	obvconfig.WriteHex("pinSameNetTextColor", byte4swap(this->pinSameNetTextColor));
	obvconfig.WriteHex("pinSameNetFillColor", byte4swap(this->pinSameNetFillColor));
	obvconfig.WriteHex("pinHaloColor", byte4swap(this->pinHaloColor));
	obvconfig.WriteHex("pinNetWebColor", byte4swap(this->pinNetWebColor));
	obvconfig.WriteHex("pinNetWebOSColor", byte4swap(this->pinNetWebOSColor));
	obvconfig.WriteHex("annotationPopupTextColor", byte4swap(this->annotationPopupTextColor));
	obvconfig.WriteHex("annotationPopupBackgroundColor", byte4swap(this->annotationPopupBackgroundColor));
	obvconfig.WriteHex("annotationBoxColor", byte4swap(this->annotationBoxColor));
	obvconfig.WriteHex("annotationStalkColor", byte4swap(this->annotationStalkColor));
	obvconfig.WriteHex("selectedMaskOutline", byte4swap(this->selectedMaskOutline));
	obvconfig.WriteHex("selectedMaskParts", byte4swap(this->selectedMaskParts));
	obvconfig.WriteHex("selectedMaskPins", byte4swap(this->selectedMaskPins));
	obvconfig.WriteHex("orMaskPins", byte4swap(this->orMaskPins));
	obvconfig.WriteHex("orMaskParts", byte4swap(this->orMaskParts));
	obvconfig.WriteHex("orMaskOutline", byte4swap(this->orMaskOutline));
}
