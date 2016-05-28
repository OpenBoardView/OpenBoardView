#include "PartList.h"

#include "imgui\imgui.h"


PartList::PartList(TcharStringCallback cbNetSelected)
{
    m_cbNetSelected = cbNetSelected;
}

PartList::~PartList()
{
}

void PartList::Draw(const char* title, bool* p_open, Board* board) {
    // TODO: export / fix dimensions & behaviour
    int width = 400;
    int height = 640;

    ImGui::SetNextWindowSize(ImVec2(width, height));
    ImGui::Begin("Part List");

    ImGui::Columns(2, "part_infos");
    ImGui::Separator();

    ImGui::Text("Name"); ImGui::NextColumn();
    ImGui::Text("Alias"); ImGui::NextColumn();
    ImGui::Separator();

    if (board) {
        std::vector<BRDPart> parts = board->Parts();

        ImGuiListClipper clipper(parts.size(), ImGui::GetTextLineHeight());
        static int selected = -1;
        char* part_name = "";
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            part_name = parts[i].name;
            char *part_annotation = parts[i].annotation;

            if (ImGui::Selectable(part_name, selected == i,
                ImGuiSelectableFlags_AllowDoubleClick))
            {
                selected = i;
                if (ImGui::IsMouseDoubleClicked(0)) {
                    m_cbNetSelected(part_name);
                }
            }
            ImGui::NextColumn();

            if (ImGui::Selectable(part_annotation, selected == i,
                ImGuiSelectableFlags_AllowDoubleClick))
            {
                selected = i;
                if (ImGui::IsMouseDoubleClicked(0)) {
                    //TODO: change field?
                }
            }
            ImGui::NextColumn();
        }
        clipper.End();
    }
    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::End();
}
