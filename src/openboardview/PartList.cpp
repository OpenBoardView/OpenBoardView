#include "PartList.h"

#include "imgui/imgui.h"

PartList::PartList(KeyBindings &keyBindings, TcharStringCallback cbNetSelected) : keyBindings(keyBindings) {
	cbNetSelected_ = cbNetSelected;
}

PartList::~PartList() {}

void PartList::Draw(const char *title, bool *p_open, Board *board) {
	// TODO: export / fix dimensions & behaviour
	int width  = 400;
	int height = 640;

	ImGui::SetNextWindowSize(ImVec2(width, height));
	ImGui::Begin(title, p_open);

	ImGui::Columns(1, "part_infos");
	ImGui::Separator();

	ImGui::Text("Name");
	ImGui::NextColumn();
	ImGui::Separator();

	if (board) {
		auto parts = board->Components();

		static int selected = -1;
		std::string part_name    = "";
		ImGuiListClipper clipper;
		clipper.Begin(parts.size());
		while (clipper.Step()) {
			for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
				part_name = parts[i]->name;

				if (ImGui::Selectable(part_name.c_str(), selected == i, ImGuiSelectableFlags_AllowDoubleClick)) {
					selected = i;
					if (ImGui::IsMouseDoubleClicked(0)) {
						cbNetSelected_(part_name.c_str());
					}
				}
				ImGui::NextColumn();
			}
		}
		clipper.End();
	}
	ImGui::Columns(1);
	ImGui::Separator();

	if (ImGui::IsWindowHovered() && keyBindings.isPressed("CloseDialog")) {
		*p_open = false;
	}

	ImGui::End();
}
