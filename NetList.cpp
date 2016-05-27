#include "NetList.h"
#include "imgui\imgui.h"

NetList::NetList(TcharStringCallback cbNetSelected)
{
	m_cbNetSelected = cbNetSelected;
}

NetList::~NetList()
{
}


void NetList::Draw(const char* title, bool* p_open, Board* board) {
	// TODO: export / fix dimensions & behaviour
	int width = 400;
	int height = 640;

	ImGui::SetNextWindowSize(ImVec2(width, height));
	ImGui::Begin("Net List");

	ImGui::Columns(1, "part_infos");
	//ImGui::Columns(2, "part_infos");
	ImGui::Separator();

	ImGui::Text("Name"); ImGui::NextColumn();
	//ImGui::Text("Alias"); ImGui::NextColumn();
	ImGui::Separator();

	if (board) {
		vector<BRDNail> nails = board->getUniqueNetList();

		ImGuiListClipper clipper(nails.size(), ImGui::GetTextLineHeight());
		static int selected = -1;
		char* net_name = "";
		for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
			net_name = nails[i].net;
			if (ImGui::Selectable(net_name, selected == i,
				ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
				selected = i;
				if (ImGui::IsMouseDoubleClicked(0)) {
					m_cbNetSelected(net_name);
				}
			}
			ImGui::NextColumn();

			//ImGui::Text("none"); ImGui::NextColumn();
		}
		clipper.End();
	}
	ImGui::Columns(1);
	ImGui::Separator();

	ImGui::End();
}
