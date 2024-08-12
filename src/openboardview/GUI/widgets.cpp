#include "widgets.h"

#include "imgui/imgui.h"

void RightAlignedText(const char *t, int w) {
	ImVec2 s = ImGui::CalcTextSize(t);
	s.x      = w - s.x;
	ImGui::Dummy(s);
	ImGui::SameLine();
	ImGui::Text("%s", t);
}
