#ifndef _IMGUIRENDERERSDLGLES2_H_
#define _IMGUIRENDERERSDLGLES2_H_

#include "ImGuiRendererSDL.h"

class ImGuiRendererSDLGLES2: public ImGuiRendererSDL {
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
