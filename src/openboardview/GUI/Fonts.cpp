#include "platform.h"

#include "Fonts.h"

#include <cmath>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

#include "imgui/imgui.h"

#include "Renderers/Renderers.h"
#include "DPI.h"
#include "utils.h"

constexpr const float Fonts::MAX_FONT_SIZE;

std::string Fonts::load(std::string customFont) {
	// Font selection
	std::deque<std::string> fontList(
	    {"Roboto", "Liberation Sans", "DejaVu Sans", "Arial", "Helvetica", ""}); // Empty string = use system default font

	if (!customFont.empty()) fontList.push_front(customFont);

	ImGuiIO &io = ImGui::GetIO();

	for (const auto &name : fontList) {
#ifdef _WIN32
		std::vector<char> ttf = load_font(name);

		ImFontConfig font_cfg{};
		std::strncpy(font_cfg.Name, name.c_str(), sizeof(font_cfg.Name));
		font_cfg.Name[sizeof(font_cfg.Name) - 1] = '\0';
		font_cfg.FontData = ttf.data();
		font_cfg.FontDataSize = ttf.size();
		font_cfg.FontDataOwnedByAtlas = false;

		if (!ttf.empty()) {
			if (io.Fonts->AddFont(&font_cfg) != nullptr) {
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Loaded font: %s", name.c_str());
				return name;
			} else {
				SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot load font %s", name.c_str());
			}
		} else {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot load font %s: not found", name.c_str());
		}
#else
		const std::string fontpath = get_font_path(name);
		if (fontpath.empty()) {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot load font %s: not found", name.c_str());
			continue;
		}
		// ImGui handles TrueType and OpenType fonts, exclude anything which has a different ext
		if (check_fileext(fontpath, ".ttf") || check_fileext(fontpath, ".otf")) {
			io.Fonts->AddFontFromFileTTF(fontpath.c_str());
			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Loaded font: %s, path: %s", name.c_str(), fontpath.c_str());
			return name;
		} else {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot load font %s: file name %s does not have .ttf or .otf extension", name.c_str(), fontpath.c_str());
		}
#endif
	}
	return {};
}

std::string Fonts::reload(std::string customFont) {
	ImGuiIO &io = ImGui::GetIO();
	io.Fonts->Clear();
	return this->load(customFont);
}
