#ifndef _IMGUIRENDERERSDLGL1_H_
#define _IMGUIRENDERERSDLGL1_H_

#include "ImGuiRendererSDL.h"

class ImGuiRendererSDLGL1: public ImGuiRendererSDL {
	using ImGuiRendererSDL::ImGuiRendererSDL;
public:
	bool checkGLVersion();
	void setGLVersion();
	bool init();
	void processEvent(SDL_Event &event);
	void initFrame();
	void shutdown();
};

#endif
