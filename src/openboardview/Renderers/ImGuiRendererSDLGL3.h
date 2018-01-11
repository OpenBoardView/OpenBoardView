#ifndef _IMGUIRENDERERSDLGL3_H_
#define _IMGUIRENDERERSDLGL3_H_

#include "ImGuiRendererSDL.h"

class ImGuiRendererSDLGL3: public ImGuiRendererSDL {
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
