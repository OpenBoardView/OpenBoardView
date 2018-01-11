#include "ImGuiRendererSDL.h"

#include <sstream>
#include <glad/glad.h>

ImGuiRendererSDL::ImGuiRendererSDL(SDL_Window *window) : window(window) {
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
}

ImGuiRendererSDL::~ImGuiRendererSDL() {
	SDL_GL_DeleteContext(glcontext);
	SDL_GL_UnloadLibrary();
}

void ImGuiRendererSDL::setGLVersion() {
	SDL_GL_ResetAttributes();
}

bool ImGuiRendererSDL::init() {
	this->setGLVersion();
	SDL_GL_LoadLibrary(nullptr);
	if (window != nullptr)
		glcontext = SDL_GL_CreateContext(window);

	if (glcontext == nullptr) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to initialize OpenGL context: %s\n", SDL_GetError());
		return false;
	}

	if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "glad failed to load OpenGL\n");
		return false;
	}

        /* Query OpenGL device information */
	const GLubyte *strrenderer = glGetString(GL_RENDERER);
	const GLubyte *vendor      = glGetString(GL_VENDOR);
	const GLubyte *version     = glGetString(GL_VERSION);
	const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

	std::stringstream ss;
	ss << "\n-------------------------------------------------------------\n";
	ss << "GL Vendor     : " << vendor;
	ss << "\nGL GLRenderer : " << strrenderer;
	ss << "\nGL Version    : " << version;
	ss << "\nGLSL Version  : " << glslVersion;
	ss << "\n-------------------------------------------------------------\n";
	SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "%s", ss.str().c_str());

	return true;
}

void ImGuiRendererSDL::renderFrame(const ImVec4 &clear_color) {
	glViewport(0, 0, static_cast<int>(ImGui::GetIO().DisplaySize.x), static_cast<int>(ImGui::GetIO().DisplaySize.y));
	glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui::Render();
	SDL_GL_SwapWindow(window); 
}
