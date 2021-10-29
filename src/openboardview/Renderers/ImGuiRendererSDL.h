#ifndef _IMGUIRENDERERSDL_H_
#define _IMGUIRENDERERSDL_H_

#include <string>

// SDL, glad
#include <SDL.h>
#include <glad/glad.h>

#include "imgui/imgui.h"

#include "filesystem_impl.h"

class ImGuiRendererSDL {
public:
	explicit ImGuiRendererSDL(SDL_Window *window);
	virtual ~ImGuiRendererSDL();

	virtual std::string name();
	virtual bool checkGLVersion() = 0;
	virtual bool init();
	virtual void processEvent(SDL_Event &event);
	virtual void initFrame();
	virtual void renderFrame(const ImVec4 &clear_color);
	virtual void renderDrawData() = 0;
	virtual void shutdown();
	virtual void unloadTexture(GLuint t, unsigned char * buffer);
	
	// Returned string is error message, empty if successful
	virtual std::string loadTextureFromFile(const filesystem::path &filepath, GLuint* out_texture, int* out_width, int* out_height);

	virtual std::string loadTextureFromData(unsigned char * image_data, int image_data_size, int * image_width, int * image_height, GLuint * out_texture, unsigned char ** image_buffer, bool use_image_loader);
 protected:
	SDL_Window *window = nullptr;
	virtual void setGLVersion();
private:
	SDL_GLContext glcontext = nullptr;
};

#endif
