#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <array>
#include <string>
#include <atomic>
#include <thread>
#include <optional>
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
	std::atomic<unsigned char *> * imgdata_raw_ = nullptr;
	std::thread * imgdata_raw_thread_ = nullptr;
	unsigned char * imgdata_thumb_ = nullptr;
	GLuint thumb_texture = 0;

	float w_aspect_ = 0.0f;

	int width = 0;
	int height = 0;
	int offsetX = 0;
	int offsetY = 0;
	float scalingX = 1.0f;
	float scalingY = 1.0f;
	bool mirrorX = false;
	bool mirrorY = false;
	float transparency = 0.0f;

	std::array<ImVec2, 4> tex_coord_ = { { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } } };
	
	std::array<ImVec2, 4> TransformRelativeCoordinates(int rotation, ImVec2 & pmin, ImVec2 & pmax);

	unsigned char * imgdata = nullptr, * imgdata_loader = nullptr;
	int imgwidth = 0;
	int imgheight = 0;
	int imgdatasize = 0;
	bool use_loader_ = false;
	bool no_rotation_ = false;
	unsigned char * thumb_ = nullptr;
	int thumb_width_ = 0;
	int thumb_height_ = 0;
	float x_right_ = 1.0f;

	float current_view_aspect_ = 1.0f;
	ImVec2 origin_ = { 0.0f, 0.0f };
	float scale_ = 1.0f;
	
	float zoom_factor_ = 1.0f;
	void observe_border();

 public:
	Image() { imgdata_raw_ = new std::atomic<unsigned char *>; }
	Image(const filesystem::path &file, bool no_rotation = false);
	Image(unsigned char * data, int dlen, int width, int height, bool use_loader, bool no_rotation = false, unsigned char * thumb = nullptr, int thumb_width = 0, int thumb_height = 0);
	~Image();
	ImVec2 CoordToScreen(ImVec2 const & xy);
	ImVec2 ScreenToCoord(ImVec2 const & xy);

	//ImVec2 center();
	//float  zoom();

	void decode_image(bool);
	
	void zoom_factor(float f) { zoom_factor_ = f; }

	void unload();
	std::string reload(bool force = false);
	std::optional<float> render(ImDrawList &draw, const ImVec2 &p_min, const ImVec2 &p_max, int rotation);
	void zoom(float s) { scale_ = s; }
	void origin(ImVec2 const & o) { origin_ = o; }
	ImVec2 origin() const { return origin_; }
	float zoom() const { return scale_; }
	void zoom(ImVec2 xz, float z);
	void scroll(ImVec2);
	ImVec2 a_scale() const;
	
	float x0() const;
	float y0() const;
	float x1() const;
	float y1() const;

	friend Preferences::BackgroundImage;
	friend class BackgroundImage;
};

#endif
