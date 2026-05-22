#ifndef _IMGUIRENDERERSDLGL3_H_
#define _IMGUIRENDERERSDLGL3_H_

#include <string>

#include "ImGuiRendererSDL.h"

#if defined(IMGUI_IMPL_OPENGL_ES2)
#define GL_VERSION_MAJOR 2
#define GL_VERSION_MINOR 0
#elif defined(IMGUI_IMPL_OPENGL_ES3)
#define GL_VERSION_MAJOR 3
#define GL_VERSION_MINOR 0
#else
#define GL_VERSION_MAJOR 3
#define GL_VERSION_MINOR 2
#endif

class ImGuiRendererSDLGL3 : public ImGuiRendererSDL {
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

private:
	std::string glsl_version;
};

#endif
