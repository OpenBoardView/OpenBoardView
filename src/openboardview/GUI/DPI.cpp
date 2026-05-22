#include "DPI.h"

#include "imgui/imgui.h"

static int dpi = 0;

// Scale factor from the platform
static float display_scale = 1.0f;

float DPIF(float x) {
	return (x * dpi * display_scale) / 100.f;
}

int DPI(int x) {
	return (x * dpi * display_scale) / 100;
}

// Inverse DPIF
float IDPIF(float x) {
	return (x * 100.0f) / (dpi * display_scale);
}

void setDPI(int new_dpi) {
	static float defaultScrollbarSize = ImGui::GetStyle().ScrollbarSize;
	dpi = new_dpi;
	ImGuiStyle &style = ImGui::GetStyle();
	style.ScrollbarSize = (defaultScrollbarSize * dpi) / 100.0f;
}

void setDisplayScale(float v) {
	display_scale = v;
}

int getDPI() {
	return dpi;
}
