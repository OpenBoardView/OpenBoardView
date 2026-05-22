#include "Controls.h"

#include <string>

#include "imgui/imgui.h"

#include "GUI/DPI.h"
#include "version.h"

namespace Help {

Controls::Controls(KeyBindings &keybindings) : keybindings(keybindings) {
}

void Controls::menuItem() {
	if (ImGui::MenuItem("Controls")) {
		shown = true;
	}
}

void Controls::render() {
	bool p_open = true;
	auto &io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), 0, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(DPI(540), 0.0f));
	if (ImGui::BeginPopupModal("Controls", &p_open)) {
		shown = false;

		ImGui::Text("Keyboard controls");
		ImGui::Separator();

		if (ImGui::BeginTable("KeyboardControls", 2, ImGuiTableFlags_BordersInnerV)) {
			ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed); // size according to content
			ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed); // size according to content

			for (auto &desc : keybindings.descriptions) {
				ImGui::TableNextRow();
				if (desc.second.empty()) {
					ImGui::TableSetColumnIndex(0);
					ImGui::Dummy(ImVec2(0.0f, DPI(5)));
					ImGui::TableSetColumnIndex(1);
					ImGui::Dummy(ImVec2(0.0f, DPI(5)));
				} else {
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%s", desc.second.c_str());
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%s", keybindings.getKeyNames(desc.first).c_str());
				}
			}
			ImGui::EndTable();
		}
		ImGui::Separator();
		ImGui::Text("Mouse controls");
		ImGui::Separator();

		if (ImGui::BeginTable("MouseControls", 2, ImGuiTableFlags_BordersInnerV)) {
			ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed); // size according to content
			ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed); // size according to content

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Highlight pins on network");
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("Click (on pin)");

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Move board");
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("Click and drag");

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Zoom (CTRL for finer steps)");
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("Scroll");

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Flip board");
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("Middle click");

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Annotations menu");
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("Right click");

			ImGui::EndTable();
		}

		if (ImGui::Button("Close") || keybindings.isPressed("CloseDialog")) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if (shown) {
		ImGui::OpenPopup("Controls");
	}
}

} // namespace Help
