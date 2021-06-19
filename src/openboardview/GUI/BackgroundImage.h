#ifndef _BACKGROUNDIMAGE_H_
#define _BACKGROUNDIMAGE_H_

#include <array>
#include <string>
#include <glad/glad.h>

#include "imgui/imgui.h"

#include "Renderers/ImGuiRendererSDL.h"
#include "GUI/Image.h"
#include "filesystem_impl.h"

// Requires forward declaration in order to friend since it's from another namespace
namespace Preferences {
	class BackgroundImage;
}

// Manages background image configuration attached to boardview file and currently shown image depending on current board side
class BackgroundImage {
public:
	enum class Side {
		TOP,
		BOTTOM
	};
private:
	static constexpr const Side defaultSide = Side::TOP; // c++ <17 need definition in .cpp

	Image topImage{};
	Image bottomImage{};

	filesystem::path configFilepath{};

	const Side *side = &defaultSide; // Try to keep this pointer always valid, linked to the BoardView::m_current_side attribute
	const Image &selectedImage() const;

	std::string error{};
public:
	BackgroundImage(const Side &side);
	BackgroundImage(const int &side);

	void loadFromConfig(const filesystem::path &filepath);
	void writeToConfig(const filesystem::path &filepath);
	std::string reload();
	void render(ImDrawList &draw, const ImVec2 &p_min, const ImVec2 &p_max, int rotation);

	bool enabled = true;

	float x0() const;
	float y0() const;
	float x1() const;
	float y1() const;

	friend Preferences::BackgroundImage;
};

#endif
