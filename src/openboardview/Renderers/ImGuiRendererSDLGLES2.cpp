#include "ImGuiRendererSDLGLES2.h"

#include "imgui_impl_sdl_gles2.h"

bool ImGuiRendererSDLGLES2::checkGLVersion() {
	return GLVersion.major >= 2 && GLVersion.minor >= 0;
}

void ImGuiRendererSDLGLES2::setGLVersion() {
	ImGuiRendererSDL::setGLVersion();
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
}

bool ImGuiRendererSDLGLES2::init() {
	ImGuiRendererSDL::init();
	return ImGui_ImplSdlGLES2_Init(window);
}

void ImGuiRendererSDLGLES2::processEvent(SDL_Event &event) {
	 ImGui_ImplSdlGLES2_ProcessEvent(&event);
}

void ImGuiRendererSDLGLES2::initFrame() {
	ImGui_ImplSdlGLES2_NewFrame();
}

void ImGuiRendererSDLGLES2::shutdown() {
	ImGui_ImplSdlGLES2_Shutdown();
}
