#include "ImGuiRendererSDL.h"

#include <sstream>
#include <glad/glad.h>

#include "imgui_impl_sdl.h"

ImGuiRendererSDL::ImGuiRendererSDL(SDL_Window *window) : window(window) {
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
}

ImGuiRendererSDL::~ImGuiRendererSDL() {
	SDL_GL_DeleteContext(glcontext);
	SDL_GL_UnloadLibrary();
}

std::string ImGuiRendererSDL::name() {
	return "ImGuiRendererSDL";
}

void ImGuiRendererSDL::setGLVersion() {
	SDL_GL_ResetAttributes();
}

bool ImGuiRendererSDL::init() {
	SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "Initializing %s", this->name().c_str());

	this->setGLVersion();
	SDL_GL_LoadLibrary(nullptr);
	if (window != nullptr)
		glcontext = SDL_GL_CreateContext(window);

	if (glcontext == nullptr) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s: Failed to initialize OpenGL context: %s\n", this->name().c_str(), SDL_GetError());
		return false;
	}

	if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s: glad failed to load OpenGL\n", this->name().c_str());
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

	ImGui_ImplSDL2_InitForOpenGL(window, glcontext);

	return true;
}

void ImGuiRendererSDL::processEvent(SDL_Event &event) {
	ImGui_ImplSDL2_ProcessEvent(&event);
}

void ImGuiRendererSDL::initFrame() {
	ImGui_ImplSDL2_NewFrame(window);
}

void ImGuiRendererSDL::renderFrame(const ImVec4 &clear_color) {
	SDL_GL_MakeCurrent(window, glcontext);
	glViewport(0, 0, static_cast<int>(ImGui::GetIO().DisplaySize.x), static_cast<int>(ImGui::GetIO().DisplaySize.y));
	glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT);
	renderDrawData();
	SDL_GL_SwapWindow(window);
}

void ImGuiRendererSDL::shutdown() {
	ImGui_ImplSDL2_Shutdown();
}
