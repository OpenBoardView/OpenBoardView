#ifndef _IMGUIRENDERERSDL_H_
#define _IMGUIRENDERERSDL_H_

// SDL, glad
#ifdef _MSC_VER
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include <glad/glad.h>

#include "imgui/imgui.h"

class ImGuiRendererSDL {
public:
	explicit ImGuiRendererSDL(SDL_Window *window);
	virtual ~ImGuiRendererSDL();

	virtual bool checkGLVersion() = 0;
	virtual bool init();
	virtual void processEvent(SDL_Event &event) = 0;
	virtual void initFrame() = 0;
	virtual void renderFrame(const ImVec4 &clear_color);
	virtual void shutdown() = 0;
protected:
	SDL_Window *window = nullptr;
	virtual void setGLVersion();
private:
	SDL_GLContext glcontext = nullptr;
};

#endif
