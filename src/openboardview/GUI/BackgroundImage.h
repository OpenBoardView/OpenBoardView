#ifndef _BACKGROUNDIMAGE_H_
#define _BACKGROUNDIMAGE_H_

#include <array>
#include <string>
#include <glad/glad.h>

#include "imgui/imgui.h"

#include "Renderers/ImGuiRendererSDL.h"
#include "GUI/Image.h"

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

	std::string configFilename{};

	const Side *side = &defaultSide; // Try to keep this pointer always valid, linked to the BoardView::m_current_side attribute
	const Image &selectedImage() const;
public:
	BackgroundImage(const Side &side);
	BackgroundImage(const int &side);

	void loadFromConfig(const std::string &filename);
	void writeToConfig(const std::string &filename);
	bool reload();
	void render(ImDrawList &draw, const ImVec2 &p_min, const ImVec2 &p_max, int rotation) const;

	float x0() const;
	float y0() const;
	float x1() const;
	float y1() const;

	friend Preferences::BackgroundImage;
};

#endif
