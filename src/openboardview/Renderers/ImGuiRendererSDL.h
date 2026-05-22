#ifndef _IMGUIRENDERERSDL_H_
#define _IMGUIRENDERERSDL_H_

#include <string>

// SDL, glad
#ifdef ENABLE_GLES2
#include <glad/gles2.h>
#else
#include <glad/gl.h>
#endif
#include <SDL.h>

#include <imgui.h>

#include "filesystem_impl.h"

class ImGuiRendererSDL {
public:
	static float getDisplayScale();

	explicit ImGuiRendererSDL();
	virtual ~ImGuiRendererSDL();

	virtual std::string name();
	virtual bool checkGLVersion(int version) = 0;
	virtual bool init();
	virtual void processEvent(SDL_Event &event);
	virtual void initFrame();
	virtual void renderFrame(const ImVec4 &clear_color);
	virtual void renderDrawData() = 0;
	virtual void shutdown();

	virtual std::string createTexture(ImTextureID* out_texture, uint8_t* data, int w, int h, char fmt) = 0;
	std::string loadTextureFromFile(const filesystem::path &filepath, ImTextureID* out_texture, int* out_width, int* out_height);
	virtual void deleteTexture(ImTextureID tex);

	virtual void toggleFullScreen();
	virtual SDL_Window *getWindow();

protected:
	SDL_Window *window = nullptr;
	int version = 0;
	int glMaxTextureSize = 0;

	virtual void setGLVersion();

private:
	SDL_GLContext glcontext = nullptr;
};

#endif
