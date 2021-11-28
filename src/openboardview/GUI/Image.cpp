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
#if 0
	if (! thumb_texture && thumb_) {
		int w = thumb_width_;
		int h = thumb_height_;
		unsigned char * p;
		Renderers::current->loadTextureFromData(thumb_, imgdatasize * 0, &w, &h, &thumb_texture, &p, false);
	}		
#endif
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

std::array<ImVec2, 4> Image::TransformRelativeCoordinates(int rotation, ImVec2 & bmin, ImVec2 & bmax) {
	std::array<ImVec2, 4> ret;

	float waspect = float(imgwidth) / float(imgheight);

	ImVec2 bdim = bmax - bmin;

	float baspect = bdim.x / bdim.y;
	current_view_aspect_ = baspect;
	
	float xscale = scale_ / waspect * baspect; // / (baspect < 1.0f ? baspect : 1.0f);
	float yscale = scale_;//  / baspect; /// (baspect > 1.0f ? baspect : 1.0f);

	//float xscale = a_scale().x / (baspect < 1.0f ? baspect : 1.0f);
	//float yscale = a_scale().y / (baspect > 1.0f ? baspect : 1.0f);
	
	if (origin_.x < 0.0f) {
		bmin.x -= bdim.x / xscale * origin_.x;
	}
	if (origin_.y < 0.0f) {
		bmin.y -= bdim.y / yscale * origin_.y;
	}
	if (xscale + origin_.x > 1.0f) {
		bmax.x -= bdim.x / xscale * (xscale - 1.0f + origin_.x);
	}
	if (yscale + origin_.y > 1.0f) {
		bmax.y -= bdim.y / yscale * (yscale - 1.0f + origin_.y);
	}

	ret[0] = { 0.0f, 0.0f };
	ret[1] = { 1.0f, 0.0f };
	ret[2] = { 1.0f, 1.0f };
	ret[3] = { 0.0f, 1.0f };

	ret[0] = origin_;
	ret[1] = { origin_.x + xscale, origin_.y };
	ret[2] = { origin_.x + xscale, origin_.y + yscale };
	ret[3] = { origin_.x, origin_.y + yscale };

	for (auto & r : ret) {
		r.x = std::clamp(r.x, 0.0f, 1.0f);
		r.y = std::clamp(r.y, 0.0f, 1.0f);
	}
	return ret;
	
	ret[1] = { origin_.x + 1.0f / (bdim.x * scale_), origin_.y };
	ret[2] = { origin_.x + 1.0f / (bdim.x * scale_), origin_.y + 1.0f / (bdim.y * scale_) };
	ret[3] = { origin_.x, origin_.y + 1.0f / (bdim.y * scale_) };
	return ret;



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

ImVec2 Image::a_scale() const {
	float waspect = float(imgwidth) / float(imgheight);
	float xscale = scale_ / waspect * current_view_aspect_;
	return ImVec2(xscale, scale_);
}

void Image::scroll(ImVec2 dxy) {
	origin_ -= dxy * a_scale();
}

void Image::zoom(ImVec2 xy, float dz) {
	ImVec2 coord = ScreenToCoord(xy);
	
	scale_ *= powf(1.3f, -dz);
	ImVec2 coord2 = ScreenToCoord(xy);

	origin_ += coord - coord2;
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
	float waspect = float(imgwidth) / float(imgheight);
	float xscale = scale_ / waspect * current_view_aspect_;
	
	ImVec2 ret = (xy - origin_);
	ret.x /= xscale;
	ret.y /= scale_;

	return ret;
}
ImVec2 Image::ScreenToCoord(ImVec2 const & xy) {
	float waspect = float(imgwidth) / float(imgheight);
	float xscale = scale_ / waspect * current_view_aspect_;

	ImVec2 ret = xy;
	ret.x *= xscale;
	ret.y *= scale_;
	return ret + origin_;
}


void Image::decode_image(bool bg) {
	if (imgdata) {
		if (! *imgdata_raw_) {
			auto fn = [this, bg]() {
				imgwidth = 0;
				imgheight = 0;
				unsigned char * tp = stbi_load_from_memory(imgdata, imgdatasize, &imgwidth, &imgheight, NULL, 4);
				if (! tp) {
					std::cerr << "decode failed\n";
				}
				*imgdata_raw_= tp;
				if (bg) {
					BoardView::wakeup();
				}
			};

			if (bg) {
				if (imgdata && ! imgdata_raw_thread_) {
					imgdata_raw_thread_ = new std::thread(fn);
				}
			} else {
				fn();
			}
		}
	}
}


std::optional<float> Image::render(ImDrawList &draw, const ImVec2 &p_min, const ImVec2 &p_max, int rotation) {
	if (! imgdata_raw_) {
		if (imgdata_raw_thread_) {
			imgdata_raw_thread_->join();
			imgdata_raw_thread_ = nullptr;
		} else {
			decode_image(true);
			return std::optional<float>();
		}
	}
	if (!texture) {
		reload(false);
	}

	if (texture) {
		draw.AddRectFilled(p_min, p_max, ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 1.0f)));
		ImVec2 p_min_new = p_min, p_max_new = p_max;

		auto uvs = TransformRelativeCoordinates(rotation, p_min_new, p_max_new);
		{
			ImVec2 pdim = p_max_new - p_min_new;

			draw.AddImageQuad(reinterpret_cast<void*>(texture),
							  p_min_new, // Asbolute render rectangle top-left corner
							  { p_min_new.x + pdim.x, p_min_new.y }, //ImVec2(p_max[0], p_min[1]), // Asbolute render rectangle top-right corner
							  { p_min_new.x + pdim.x, p_min_new.y + pdim.y }, //p_max, // Absolute render rectangle bottom-right corner
							  { p_min_new.x, p_min_new.y + pdim.y }, //ImVec2(p_min[0], p_max[1]), // Absolute render rectangle bottom-left corner
							  
							  uvs[0], // Image relative top-left corner
							  uvs[1], // Image relative top-right corner
							  uvs[2], // Image relative bottom-right corner
							  uvs[3], // Image relative bottom-left corner
							  ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f - transparency)));
			
			return scale_ * imgwidth * imgheight / (pdim.x * pdim.y);
		}
	}
	return std::optional<float>();
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
