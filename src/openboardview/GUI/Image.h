#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <array>
#include <string>
#include <glad/glad.h>

#include "imgui/imgui.h"

#include "filesystem_impl.h"
#include "Renderers/ImGuiRendererSDL.h"

// Requires forward declaration in order to friend since it's from another namespace
namespace Preferences {
	class BackgroundImage;
}

class Image {
private:
	filesystem::path file{};
	GLuint texture = 0;
	int width = 0;
	int height = 0;
	int offsetX = 0;
	int offsetY = 0;
	float scalingX = 1.0f;
	float scalingY = 1.0f;
	bool mirrorX = false;
	bool mirrorY = false;
	float transparency = 0.0f;

	std::array<ImVec2, 4> TransformRelativeCoordinates(int rotation) const;

public:
	Image() = default;
	Image(const filesystem::path &file);
	~Image();

	std::string reload();
	void render(ImDrawList &draw, const ImVec2 &p_min, const ImVec2 &p_max, int rotation) const;

	float x0() const;
	float y0() const;
	float x1() const;
	float y1() const;

	friend Preferences::BackgroundImage;
	friend class BackgroundImage;
};

#endif
