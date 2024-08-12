#include "platform.h"

#include "Fonts.h"

#include <cmath>
#include <deque>
#include <string>
#include <vector>

#include "imgui/imgui.h"

#include "Renderers/Renderers.h"
#include "DPI.h"
#include "utils.h"

std::string Fonts::load(std::string customFont, double fontSize) {
	fontSize = (fontSize * getDPI()) / 100;

	// Font selection
	std::deque<std::string> fontList(
	    {"Liberation Sans", "DejaVu Sans", "Arial", "Helvetica", ""}); // Empty string = use system default font

	if (!customFont.empty()) fontList.push_front(customFont);

	// Try to keep atlas size below 1024x1024 pixels (max texture size on e.g. Windows GDI)
	ImGuiIO &io = ImGui::GetIO();
	io.Fonts->TexDesiredWidth = 1024;
	// Max size is 76px for a single font with current glyph range (192 glyphs) and default oversampling (h*3, v*1), could break in future ImGui updates
	// Different fonts can also overflow, e.g. Arial and Helvetica work but Algerian does not
	// Expected max fontSize is around 50, limit set in BoardView.cpp for Preferences panel, user can manually set higher value in config file but could break
	// Rough estimate with some margin (72 instead of 76), assuming small font is 2 times smaller, max size for larger font is
	double maxLargeFontSizeSquared = 72.0 * 72.0 - fontSize * fontSize - (fontSize / 2.0) * (fontSize / 2.0);
	if (maxLargeFontSizeSquared < 1.0) {
		maxLargeFontSizeSquared = 1.0;
	}
	double largeFontScaleFactor = std::min(8.0, std::sqrt(maxLargeFontSizeSquared) / fontSize); // Find max scale factor for large font, use at most 8 times larger font

	for (const auto &name : fontList) {
#ifdef _WIN32
		ImFontConfig font_cfg{};
		font_cfg.FontDataOwnedByAtlas = false;
		const std::vector<char> ttf   = load_font(name);
		if (!ttf.empty()) {
			io.Fonts->AddFontFromMemoryTTF(
			    const_cast<void *>(reinterpret_cast<const void *>(ttf.data())), ttf.size(), fontSize, &font_cfg);
			io.Fonts->AddFontFromMemoryTTF(
			    const_cast<void *>(reinterpret_cast<const void *>(ttf.data())), ttf.size(), fontSize * largeFontScaleFactor, &font_cfg); // Larger font for resizeable (zoomed) text
			io.Fonts->AddFontFromMemoryTTF(
			    const_cast<void *>(reinterpret_cast<const void *>(ttf.data())), ttf.size(), fontSize / 2, &font_cfg); // Smaller font for resizeable (zoomed) text
			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Loaded font: %s", name.c_str());
			return name;
		} else {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot load font %s: not found", name.c_str());
		}
#else
		const std::string fontpath = get_font_path(name);
		if (fontpath.empty()) {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot load font %s: not found", name.c_str());
			continue;
		}
		if (check_fileext(fontpath, ".ttf")) { // ImGui handles only TrueType fonts so exclude anything which has a different ext
			io.Fonts->AddFontFromFileTTF(fontpath.c_str(), fontSize);
			io.Fonts->AddFontFromFileTTF(fontpath.c_str(), fontSize * largeFontScaleFactor); // Larger font for resizeable (zoomed) text
			io.Fonts->AddFontFromFileTTF(fontpath.c_str(), fontSize / 2); // Smaller font for resizeable (zoomed) text
			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Loaded font: %s", name.c_str());
			return name;
		} else {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot load font %s: file name %s does not have .ttf extension", name.c_str(), fontpath.c_str());
		}
#endif
	}
	return {};
}

std::string Fonts::reload(std::string customFont, double fontSize) {
	ImGuiIO &io = ImGui::GetIO();
	io.Fonts->Clear();
	Renderers::current->destroyFontsTexture();
	return this->load(customFont, fontSize);
}
