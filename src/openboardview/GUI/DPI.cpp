#include "DPI.h"

#include "imgui/imgui.h"

static int dpi = 0;

float DPIF(float x) {
	return (x * dpi) / 100.f;
}

int DPI(int x) {
	return (x * dpi) / 100;
}

void setDPI(int new_dpi) {
	dpi = new_dpi;
	ImGuiStyle &style = ImGui::GetStyle();
	style.ScrollbarSize *= dpi / 100.0f;
}

int getDPI() {
	return dpi;
}
