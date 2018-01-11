#include "ImGuiRendererSDLGL1.h"

#include "imgui_impl_sdl.h"

bool ImGuiRendererSDLGL1::checkGLVersion() {
	return GLVersion.major >= 1 && GLVersion.minor >= 1;
}

void ImGuiRendererSDLGL1::setGLVersion() {
	ImGuiRendererSDL::setGLVersion();
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
}

bool ImGuiRendererSDLGL1::init() {
	ImGuiRendererSDL::init();
	return ImGui_ImplSdl_Init(window);
}

void ImGuiRendererSDLGL1::processEvent(SDL_Event &event) {
	 ImGui_ImplSdl_ProcessEvent(&event);
}

void ImGuiRendererSDLGL1::initFrame() {
	ImGui_ImplSdl_NewFrame(window);
}

void ImGuiRendererSDLGL1::shutdown() {
	ImGui_ImplSdl_Shutdown();
}
