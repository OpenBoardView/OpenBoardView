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

		ImGui::Text("KEYBOARD CONTROLS");

		ImGui::Separator();

		ImGui::Columns(2);
		ImGui::PushItemWidth(-1);
		ImGui::Text("Load file");
		ImGui::Text("Quit");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("Pan up");
		ImGui::Text("Pan down");
		ImGui::Text("Pan left");
		ImGui::Text("Pan right");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("Toggle information panel");
		ImGui::Text("Search for component/Net");
		ImGui::Text("Display component list");
		ImGui::Text("Display net list");
		ImGui::Text("Clear all highlighted items");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("Mirror board");
		ImGui::Text("Flip board");
		ImGui::Text(" ");

		ImGui::Text("Zoom in");
		ImGui::Text("Zoom out");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("Reset zoom and center");
		ImGui::Text("Rotate clockwise");
		ImGui::Text("Rotate anticlockwise");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("Toggle Pin Blanking");

		/*
		 * NEXT COLUMN
		 */
		ImGui::NextColumn();
		ImGui::Text("CTRL-o");
		ImGui::Text("CTRL-q");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("w, numpad-up, arrow-up");
		ImGui::Text("s, numpad-down, arrow-down");
		ImGui::Text("a, numpad-left, arrow-left");
		ImGui::Text("d, numpad-right, arrow-right");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("i");
		ImGui::Text("CTRL-f, /");
		ImGui::Text("k");
		ImGui::Text("l");
		ImGui::Text("ESC");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("m");
		ImGui::Text("Space bar");
		ImGui::Text("(+shift to hold position)");
		ImGui::Spacing();

		ImGui::Text("+, numpad+");
		ImGui::Text("-, numpad-");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("x, numpad-5");
		ImGui::Text("'.' numpad '.'");
		ImGui::Text("',' numpad-0");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("p");

		ImGui::Columns(1);
		ImGui::Separator();

		ImGui::Text("MOUSE CONTROLS");
		ImGui::Separator();
		ImGui::Columns(2);
		ImGui::Text("Highlight pins on network");
		ImGui::Text("Move board");
		ImGui::Text("Zoom (CTRL for finer steps)");
		ImGui::Text("Flip board");
		ImGui::Text("Annotations menu");

		ImGui::NextColumn();
		ImGui::Text("Click (on pin)");
		ImGui::Text("Click and drag");
		ImGui::Text("Scroll");
		ImGui::Text("Middle click");
		ImGui::Text("Right click");
		ImGui::Columns(1);

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
