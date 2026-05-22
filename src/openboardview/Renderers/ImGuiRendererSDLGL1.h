#ifndef _IMGUIRENDERERSDLGL1_H_
#define _IMGUIRENDERERSDLGL1_H_

#include <string>

#include "ImGuiRendererSDL.h"

class ImGuiRendererSDLGL1 : public ImGuiRendererSDL {
	using ImGuiRendererSDL::ImGuiRendererSDL;

public:
	std::string name();
	bool checkGLVersion(int version);
	void setGLVersion();
	bool init();
	void initFrame();
	void renderDrawData();
	void shutdown();
	std::string createTexture(ImTextureID* out_texture, uint8_t* data, int w, int h, char fmt);
};

#endif
