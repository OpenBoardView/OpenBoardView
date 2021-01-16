#include "Image.h"

#include "imgui/imgui.h"
#include <glad/glad.h>

#include <array>

#include "Renderers/Renderers.h"

Image::Image(const filesystem::path &file) : file(file) {
}

Image::~Image() {
	glDeleteTextures(1, &texture);
}

std::string Image::reload() {
	glDeleteTextures(1, &texture);
	texture = 0;
	if (file.empty()) { // Ignore empty paths silently
		return {};
	}
	return Renderers::current->loadTextureFromFile(file, &texture, &width, &height);
}

std::array<ImVec2, 4> Image::TransformRelativeCoordinates(int rotation) const {
	switch (rotation) {
		case 0:
			return {{
				{mirrorX ? 1.0f : 0.0f, mirrorY ? 0.0f : 1.0f},
				{mirrorX ? 0.0f : 1.0f, mirrorY ? 0.0f : 1.0f},
				{mirrorX ? 0.0f : 1.0f, mirrorY ? 1.0f : 0.0f},
				{mirrorX ? 1.0f : 0.0f, mirrorY ? 1.0f : 0.0f},
			}};
			break;
		case 1:
			return {{
				{mirrorX ? 1.0f : 0.0f, mirrorY ? 0.0f : 1.0f},
				{mirrorX ? 1.0f : 0.0f, mirrorY ? 1.0f : 0.0f},
				{mirrorX ? 0.0f : 1.0f, mirrorY ? 1.0f : 0.0f},
				{mirrorX ? 0.0f : 1.0f, mirrorY ? 0.0f : 1.0f},
			}};
			break;
		case 2:
			return {{
				{mirrorX ? 1.0f : 0.0f, mirrorY ? 0.0f : 1.0f},
				{mirrorX ? 0.0f : 1.0f, mirrorY ? 0.0f : 1.0f},
				{mirrorX ? 0.0f : 1.0f, mirrorY ? 1.0f : 0.0f},
				{mirrorX ? 1.0f : 0.0f, mirrorY ? 1.0f : 0.0f},
			}};
			break;
		case 3:
		default:
			return {{
				{mirrorX ? 1.0f : 0.0f, mirrorY ? 0.0f : 1.0f},
				{mirrorX ? 1.0f : 0.0f, mirrorY ? 1.0f : 0.0f},
				{mirrorX ? 0.0f : 1.0f, mirrorY ? 1.0f : 0.0f},
				{mirrorX ? 0.0f : 1.0f, mirrorY ? 0.0f : 1.0f},
			}};
			break;
	}
}

void Image::render(ImDrawList &draw, const ImVec2 &p_min, const ImVec2 &p_max, int rotation) const {
	if (!texture) { // Nothing to render
		return;
	}

	auto uvs = TransformRelativeCoordinates(rotation);

	draw.AddImageQuad(reinterpret_cast<void*>(texture),
		p_min, // Asbolute render rectangle top-left corner
		ImVec2(p_max[0], p_min[1]), // Asbolute render rectangle top-right corner
		p_max, // Absolute render rectangle bottom-right corner
		ImVec2(p_min[0], p_max[1]), // Absolute render rectangle bottom-left corner
		uvs[0], // Image relative top-left corner
		uvs[1], // Image relative top-right corner
		uvs[2], // Image relative bottom-right corner
		uvs[3], // Image relative bottom-left corner
		ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f - transparency)));
}

float Image::x0() const {
	return offsetX;
}

float Image::y0() const {
	return offsetY;
}

float Image::x1() const {
	return offsetX + (width * scalingX);
}

float Image::y1() const {
	return offsetY + (height * scalingY);
}
