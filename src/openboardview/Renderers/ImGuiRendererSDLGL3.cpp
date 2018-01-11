#include "ImGuiRendererSDLGL3.h"

#include "imgui_impl_sdl_gl3.h"

bool ImGuiRendererSDLGL3::checkGLVersion() {
	return GLVersion.major >= 3 && GLVersion.minor >= 2;
}

void ImGuiRendererSDLGL3::setGLVersion() {
	ImGuiRendererSDL::setGLVersion();
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
}

bool ImGuiRendererSDLGL3::init() {
	ImGuiRendererSDL::init();
	return ImGui_ImplSdlGL3_Init(window);
}

void ImGuiRendererSDLGL3::processEvent(SDL_Event &event) {
	 ImGui_ImplSdlGL3_ProcessEvent(&event);
}

void ImGuiRendererSDLGL3::initFrame() {
	ImGui_ImplSdlGL3_NewFrame(window);
}

void ImGuiRendererSDLGL3::shutdown() {
	ImGui_ImplSdlGL3_Shutdown();
}
