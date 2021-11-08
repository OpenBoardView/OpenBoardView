#include "Image.h"

#include "imgui/imgui.h"
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <array>
#include <algorithm>
#include <iostream>
#include "imgui_operators.h"

#include "Renderers/Renderers.h"
#include "BoardView.h"

Image::Image(const filesystem::path &file, bool no_r) : file(file), no_rotation_(no_r) {
	imgdata_raw_ = new std::atomic<unsigned char *>(nullptr);
}

Image::Image(unsigned char * data, int dlen, int width, int height, bool use_l, bool no_r, unsigned char * thumb, int thumb_width, int thumb_height) : imgdata(data), imgwidth(width), imgheight(height), imgdatasize(dlen), use_loader_(use_l), no_rotation_(no_r), thumb_(thumb), thumb_width_(thumb_width), thumb_height_(thumb_height) {
	imgdata_raw_ = new std::atomic<unsigned char *>(nullptr);
}

Image::~Image() {
	unload();
	delete *imgdata_raw_;
	//if (imgdata_thumb_) { delete[] imgdata_thumb_; }
}

void Image::unload() {
	glDeleteTextures(1, &texture);
	texture = 0;
	if (*imgdata_raw_) {
		stbi_image_free(*imgdata_raw_);
		*imgdata_raw_ = nullptr;
	}
}

std::string Image::reload(bool force) {
	if (! thumb_texture && thumb_) {
		int w = thumb_width_;
		int h = thumb_height_;
		unsigned char * p;
		Renderers::current->loadTextureFromData(thumb_, imgdatasize * 0, &w, &h, &thumb_texture, &p, false);
	}		

	if (texture && !force) return "";
	if (texture) {
		glDeleteTextures(1, &texture);
	}
	texture = 0;
	std::string ret;
	if (*imgdata_raw_) {
		unsigned char * p = nullptr;
		ret = Renderers::current->loadTextureFromData(*imgdata_raw_, imgdatasize * 0, &imgwidth, &imgheight, &texture, &p, false);
		//*imgdata_raw_ = p;
		return ret;
	} else {
		if (file.empty()) { // Ignore empty paths silently
			return "";
		}
		return Renderers::current->loadTextureFromFile(file, &texture, &width, &height);
	}
}

std::array<ImVec2, 4> Image::TransformRelativeCoordinates(int rotation) const {
	std::array<ImVec2, 4> c = tex_coord_;

	for (auto & i : c) {
		i.x *= scalingX;
		i.y *= scalingY;
	}

	if (true || ! no_rotation_) {
		if (rotation % 2) {
			std::swap(c[1], c[3]);
		}
		if (mirrorX) {
			for (auto & i : c) {
				i.x = 1.0f - i.x;
			}
		}
		if (mirrorY) {
			for (auto & i : c) {
				i.y = 1.0f - i.y;
			}
		}
	}
	return c;

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

void Image::scroll(ImVec2 dxy) {
	//dxy.y = -dxy.y;
	ImVec2 dim = { tex_coord_[2].x - tex_coord_[3].x, tex_coord_[0].y - tex_coord_[3].y };
	dxy *= dim;
	for (auto & i : tex_coord_) {
		i -= dxy;
	}
	observe_border();
}

void Image::zoom(ImVec2 xy, float dz) {
	ImVec2 coord = ScreenToCoord(xy);
	
	ImVec2 dzz { -dz / 2 * zoom_factor_, -dz / 2 * zoom_factor_ };
	
	
	for (auto & i : tex_coord_) {
		i += dzz * (i - xy);
	}

	float w_aspect = w_aspect_; //abs((p_max - p_min).x / (p_max - p_min).y);
	float i_aspect = float(imgwidth) / float(imgheight);
	
	float x_new = tex_coord_[0].x + (tex_coord_[1].x - tex_coord_[0].x) / i_aspect * w_aspect;
	x_right_ = x_new;
	
	ImVec2 off = ScreenToCoord(xy);
	for (auto & i : tex_coord_) {
		i += coord - off;
	}
	observe_border();
}

void Image::observe_border() {
	return;
	while (true) {

		ImVec2 dim = { tex_coord_[2].x - tex_coord_[3].x, tex_coord_[0].y - tex_coord_[3].y };
		if (dim.x > 1.0f || dim.y > 1.0f) {
			tex_coord_[0] = { 0.0f, 1.0f };
			tex_coord_[1] = { 1.0f, 1.0f };
			tex_coord_[2] = { 1.0f, 0.0f };
			tex_coord_[3] = { 0.0f, 0.0f };
		}
		bool scroll = false;
		ImVec2 scr;
		for (int i = 0; i < 4; ++i) {
			if (tex_coord_[i].x < 0.0f) {
				scr = { -tex_coord_[i].x, 0 };
				scroll = true;
				break;
			}
			if (tex_coord_[i].x > 1.0f) {
				scr = { -tex_coord_[i].x + 1.0f, 0 };
				scroll = true;
				break;
			}
			if (tex_coord_[i].y < 0.0f) {
				scr = { 0, -tex_coord_[i].y };
				scroll = true;
				break;
			}
			if (tex_coord_[i].y > 1.0f) {
				scr = { 0, -tex_coord_[i].y + 1.0f };
				scroll = true;
				break;
			}
		}
		if (scroll) {
			for (auto & i : tex_coord_) {
				i += scr;
			}
		} else {
			break;
		}
	}
}

ImVec2 Image::CoordToScreen(ImVec2 const & xy) {
	auto c = tex_coord_;
	//float xd = c[1].x - c[0].x;
	float xd = x_right_ - c[0].x;
	float yd = c[0].y - c[3].y;
	return { (xy.x - c[3].x) / xd, (xy.y - c[3].y) / yd };
}
ImVec2 Image::ScreenToCoord(ImVec2 const & xy) {
	auto c = tex_coord_;

	//float xd = c[1].x - c[0].x;
	float xd = x_right_ - c[0].x;
	float yd = c[0].y - c[3].y;
	return { c[3].x + xd * xy.x, c[3].y + yd * xy.y };
}

void Image::render(ImDrawList &draw, const ImVec2 &p_min, const ImVec2 &p_max, int rotation) {
	if (! *imgdata_raw_) {
		if (imgdata && ! imgdata_raw_thread_) {
			imgdata_raw_thread_ = new std::thread
				([this]() {
					imgwidth = 0;
					imgheight = 0;
					unsigned char * tp = stbi_load_from_memory(imgdata, imgdatasize, &imgwidth, &imgheight, NULL, 4);
					if (! tp) {
						std::cerr << "decode failed\n";
					}
					*imgdata_raw_= tp;
					BoardView::wakeup();
				});
		}
		if (thumb_texture) {
			auto uvs = TransformRelativeCoordinates(rotation);
			draw.AddImageQuad(reinterpret_cast<void*>(thumb_texture),
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
		return;
	}
	if (imgdata_raw_thread_) {
		imgdata_raw_thread_->join();
		imgdata_raw_thread_ = nullptr;
	}
	if (!texture) {
		reload(false);
	}
	
	if (texture) {
		auto uvs = TransformRelativeCoordinates(rotation);
#if 1
		float w_aspect = abs((p_max - p_min).x / (p_max - p_min).y);
		float i_aspect = float(imgwidth) / float(imgheight);

		float x_new = uvs[0].x + (uvs[1].x - uvs[0].x) / i_aspect * w_aspect;
		uvs[1].x = x_new; //uvs[0].x / i_aspect * w_aspect;
		uvs[2].x = x_new; //uvs[3].x / i_aspect * w_aspect;
		x_right_ = x_new;

		w_aspect_ = w_aspect;
		
		//std::cerr << w_aspect << " " << i_aspect << " " << i_aspect / w_aspect << "\n";
#endif
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
