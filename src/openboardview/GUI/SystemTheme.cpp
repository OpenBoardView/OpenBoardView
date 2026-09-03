#include "SystemTheme.h"

#include "DPI.h"
#include "imgui/imgui.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <sys/stat.h>

namespace {

std::string trim(std::string value) {
	const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c); });
	const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c); }).base();
	return first < last ? std::string(first, last) : std::string{};
}

std::map<std::string, std::string> read_palette(const std::string &path) {
	std::ifstream input(path);
	std::map<std::string, std::string> palette;
	std::string line;
	while (std::getline(input, line)) {
		line = trim(line);
		if (line.empty() || line.front() == '#') continue;
		const auto equals = line.find('=');
		if (equals == std::string::npos) continue;
		std::string key = trim(line.substr(0, equals));
		std::string value = trim(line.substr(equals + 1));
		if (value.size() >= 2 && value.front() == '"' && value.back() == '"') value = value.substr(1, value.size() - 2);
		if (!key.empty() && !value.empty()) palette[key] = value;
	}
	return palette;
}

bool parse_hex(const std::string &value, ImVec4 &color) {
	if (value.size() != 7 || value.front() != '#') return false;
	char *end = nullptr;
	const unsigned long rgb = std::strtoul(value.c_str() + 1, &end, 16);
	if (!end || *end != '\0') return false;
	color = ImVec4(static_cast<float>((rgb >> 16) & 0xff) / 255.0f,
	               static_cast<float>((rgb >> 8) & 0xff) / 255.0f,
	               static_cast<float>(rgb & 0xff) / 255.0f,
	               1.0f);
	return true;
}

ImVec4 palette_color(const std::map<std::string, std::string> &palette, const char *name, const ImVec4 &fallback) {
	const auto entry = palette.find(name);
	ImVec4 color;
	return entry != palette.end() && parse_hex(entry->second, color) ? color : fallback;
}

ImVec4 mix(const ImVec4 &from, const ImVec4 &to, float amount) {
	return ImVec4(from.x + (to.x - from.x) * amount,
	              from.y + (to.y - from.y) * amount,
	              from.z + (to.z - from.z) * amount,
	              from.w + (to.w - from.w) * amount);
}

ImVec4 alpha(ImVec4 color, float value) {
	color.w = value;
	return color;
}

float luminance(const ImVec4 &color) {
	return color.x * 0.2126f + color.y * 0.7152f + color.z * 0.0722f;
}

void apply_palette(const std::map<std::string, std::string> &palette) {
	const bool declared_dark = palette.count("mode") && palette.at("mode") == "dark";
	const ImVec4 fallback_background = declared_dark ? ImVec4(0.05f, 0.05f, 0.06f, 1.0f) : ImVec4(0.95f, 0.95f, 0.96f, 1.0f);
	const ImVec4 background = palette_color(palette, "background", fallback_background);
	const bool dark = palette.count("mode") ? declared_dark : luminance(background) < 0.5f;
	const ImVec4 foreground = palette_color(palette, "foreground", dark ? ImVec4(0.93f, 0.93f, 0.94f, 1.0f)
	                                                                                : ImVec4(0.08f, 0.08f, 0.09f, 1.0f));
	const ImVec4 accent = palette_color(palette, "accent", ImVec4(0.36f, 0.57f, 0.92f, 1.0f));
	const ImVec4 muted = palette_color(palette, "muted", mix(background, foreground, dark ? 0.28f : 0.22f));
	const ImVec4 selection = palette_color(palette, "selection", mix(background, accent, dark ? 0.42f : 0.22f));
	const ImVec4 red = palette_color(palette, "red", ImVec4(0.82f, 0.32f, 0.30f, 1.0f));
	const ImVec4 raw_yellow = palette_color(palette, "yellow", ImVec4(0.88f, 0.68f, 0.24f, 1.0f));
	const ImVec4 yellow = dark ? mix(raw_yellow, foreground, 0.28f) : raw_yellow;
	const ImVec4 green = palette_color(palette, "green", ImVec4(0.30f, 0.68f, 0.44f, 1.0f));
	const ImVec4 raw_disabled = palette_color(palette, dark ? "dark_foreground" : "light_foreground",
	                                          mix(background, foreground, dark ? 0.48f : 0.56f));
	const ImVec4 disabled = dark ? mix(raw_disabled, foreground, 0.18f) : raw_disabled;
	const ImVec4 surface = mix(background, foreground, dark ? 0.055f : 0.035f);
	const ImVec4 raised = mix(background, foreground, dark ? 0.10f : 0.075f);
	const ImVec4 accent_soft = mix(background, accent, dark ? 0.30f : 0.18f);
	const ImVec4 accent_hover = mix(background, accent, dark ? 0.48f : 0.32f);

	ImGuiStyle &style = ImGui::GetStyle();
	style.WindowPadding = ImVec2(DPIF(11.0f), DPIF(10.0f));
	style.FramePadding = ImVec2(DPIF(9.0f), DPIF(5.0f));
	style.CellPadding = ImVec2(DPIF(8.0f), DPIF(5.0f));
	style.ItemSpacing = ImVec2(DPIF(9.0f), DPIF(8.0f));
	style.ItemInnerSpacing = ImVec2(DPIF(7.0f), DPIF(5.0f));
	style.ScrollbarSize = DPIF(13.0f);
	style.GrabMinSize = DPIF(10.0f);
	style.WindowRounding = 0.0f;
	style.ChildRounding = DPIF(5.0f);
	style.FrameRounding = DPIF(4.0f);
	style.PopupRounding = DPIF(5.0f);
	style.ScrollbarRounding = DPIF(6.0f);
	style.GrabRounding = DPIF(4.0f);
	style.TabRounding = DPIF(4.0f);
	style.DisabledAlpha = 0.62f;

	ImVec4 *colors = style.Colors;
	colors[ImGuiCol_Text] = foreground;
	colors[ImGuiCol_TextDisabled] = disabled;
	colors[ImGuiCol_WindowBg] = background;
	colors[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
	colors[ImGuiCol_PopupBg] = raised;
	colors[ImGuiCol_Border] = alpha(muted, 0.72f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
	colors[ImGuiCol_FrameBg] = raised;
	colors[ImGuiCol_FrameBgHovered] = accent_soft;
	colors[ImGuiCol_FrameBgActive] = selection;
	colors[ImGuiCol_TitleBg] = background;
	colors[ImGuiCol_TitleBgActive] = surface;
	colors[ImGuiCol_TitleBgCollapsed] = background;
	colors[ImGuiCol_MenuBarBg] = surface;
	colors[ImGuiCol_ScrollbarBg] = background;
	colors[ImGuiCol_ScrollbarGrab] = muted;
	colors[ImGuiCol_ScrollbarGrabHovered] = accent_soft;
	colors[ImGuiCol_ScrollbarGrabActive] = accent_hover;
	colors[ImGuiCol_CheckMark] = green;
	colors[ImGuiCol_CheckboxSelectedBg] = selection;
	colors[ImGuiCol_SliderGrab] = accent;
	colors[ImGuiCol_SliderGrabActive] = mix(accent, foreground, 0.18f);
	colors[ImGuiCol_Button] = accent_soft;
	colors[ImGuiCol_ButtonHovered] = accent_hover;
	colors[ImGuiCol_ButtonActive] = mix(background, accent, dark ? 0.64f : 0.46f);
	colors[ImGuiCol_Header] = surface;
	colors[ImGuiCol_HeaderHovered] = accent_soft;
	colors[ImGuiCol_HeaderActive] = selection;
	colors[ImGuiCol_Separator] = alpha(muted, 0.72f);
	colors[ImGuiCol_SeparatorHovered] = accent;
	colors[ImGuiCol_SeparatorActive] = accent;
	colors[ImGuiCol_ResizeGrip] = alpha(accent, 0.20f);
	colors[ImGuiCol_ResizeGripHovered] = alpha(accent, 0.65f);
	colors[ImGuiCol_ResizeGripActive] = accent;
	colors[ImGuiCol_InputTextCursor] = foreground;
	colors[ImGuiCol_Tab] = surface;
	colors[ImGuiCol_TabHovered] = accent_soft;
	colors[ImGuiCol_TabSelected] = selection;
	colors[ImGuiCol_TabSelectedOverline] = accent;
	colors[ImGuiCol_TabDimmed] = background;
	colors[ImGuiCol_TabDimmedSelected] = surface;
	colors[ImGuiCol_TabDimmedSelectedOverline] = muted;
	colors[ImGuiCol_PlotLines] = foreground;
	colors[ImGuiCol_PlotLinesHovered] = red;
	colors[ImGuiCol_PlotHistogram] = green;
	colors[ImGuiCol_PlotHistogramHovered] = yellow;
	colors[ImGuiCol_TableHeaderBg] = surface;
	colors[ImGuiCol_TableBorderStrong] = muted;
	colors[ImGuiCol_TableBorderLight] = alpha(muted, 0.50f);
	colors[ImGuiCol_TableRowBg] = background;
	colors[ImGuiCol_TableRowBgAlt] = surface;
	colors[ImGuiCol_TextLink] = accent;
	colors[ImGuiCol_TextSelectedBg] = alpha(selection, 0.72f);
	colors[ImGuiCol_TreeLines] = muted;
	colors[ImGuiCol_DragDropTarget] = yellow;
	colors[ImGuiCol_DragDropTargetBg] = alpha(yellow, 0.18f);
	colors[ImGuiCol_UnsavedMarker] = yellow;
	colors[ImGuiCol_NavCursor] = accent;
	colors[ImGuiCol_NavWindowingHighlight] = foreground;
	colors[ImGuiCol_NavWindowingDimBg] = alpha(background, 0.72f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0, 0, 0, dark ? 0.54f : 0.32f);
}

} // namespace

std::string SystemTheme::ThemePath() const {
	if (const char *override_path = std::getenv("OPENBOARDVIEW_THEME_FILE")) {
		if (*override_path) return override_path;
	}
	const char *home = std::getenv("HOME");
	return home && *home ? std::string(home) + "/.local/state/omarchy/current/theme/colors.toml" : std::string{};
}

bool SystemTheme::ApplyIfChanged(bool force) {
	const auto now = std::chrono::steady_clock::now();
	if (!force && now < next_check_) return false;
	next_check_ = now + std::chrono::seconds(1);

	const std::string path = ThemePath();
	struct stat info {};
	if (path.empty() || stat(path.c_str(), &info) != 0) return false;
	const std::int64_t modified = static_cast<std::int64_t>(info.st_mtime);
	const std::int64_t size = static_cast<std::int64_t>(info.st_size);
	if (!force && applied_ && modified == modified_at_ && size == file_size_) return false;

	const auto palette = read_palette(path);
	if (!palette.count("background") || !palette.count("foreground")) return false;
	apply_palette(palette);
	modified_at_ = modified;
	file_size_ = size;
	applied_ = true;
	return true;
}
