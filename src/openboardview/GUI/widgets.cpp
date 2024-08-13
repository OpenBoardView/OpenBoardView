#include "widgets.h"

#include <string>

#include "imgui/imgui.h"

void RightAlignedText(const char *t, int w) {
	ImVec2 s = ImGui::CalcTextSize(t);
	s.x      = w - s.x;
	ImGui::Dummy(s);
	ImGui::SameLine();
	ImGui::Text("%s", t);
}

bool MenuItemWithCheckbox(const std::string &label, const std::string &shortcut, bool &selected, bool enabled) {
	ImGuiStyle &style = ImGui::GetStyle();

	// MenuItem is never "selected", value is toggled at the end and Checkbox is used instead to display a checkmark
	bool ret = ImGui::MenuItem(label.c_str(), shortcut.c_str(), false, enabled);
	bool hovered = ImGui::IsItemHovered();


	ImGui::SameLine();

	// Checkbox will be colored when MenuItem is hovered
	if (hovered) {
		ImGui::PushStyleColor(ImGuiCol_FrameBg, style.Colors[ImGuiCol_FrameBgHovered]);
	}

	// No padding for checkbox to fit in MenuItem height
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

	std::string id = "##C" + label;
	ImGui::Checkbox(id.c_str(), &selected);

	if (hovered) {
		ImGui::PopStyleColor();
	}

	ImGui::PopStyleVar();

	if (ret) {
		selected = !selected;
	}

	return ret;
}
