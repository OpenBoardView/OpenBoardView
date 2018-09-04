#ifndef _IMGUIRENDERERSDLGL1_H_
#define _IMGUIRENDERERSDLGL1_H_

#include "ImGuiRendererSDL.h"

class ImGuiRendererSDLGL1: public ImGuiRendererSDL {
	using ImGuiRendererSDL::ImGuiRendererSDL;
public:
	std::string name();
	bool checkGLVersion();
	void setGLVersion();
	bool init();
	void initFrame();
	void renderDrawData();
	void shutdown();
};

#endif
