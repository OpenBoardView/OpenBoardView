#include "ImGuiRendererSDLGL3.h"

#include "imgui_impl_sdl_gl3.h"

std::string ImGuiRendererSDLGL3::name() {
    return "ImGuiRendererSDLGL3";
}

bool ImGuiRendererSDLGL3::checkGLVersion() {
	if (GLVersion.major <= 3 && GLVersion.minor < 2) {
		SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Minimal OpenGL version required is %d.%d. Got %d.%d.", 3, 2, GLVersion.major, GLVersion.minor);
		return false;
	}
	return true;
}

void ImGuiRendererSDLGL3::setGLVersion() {
	ImGuiRendererSDL::setGLVersion();
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
}

bool ImGuiRendererSDLGL3::init() {
	return ImGuiRendererSDL::init() && ImGui_ImplSdlGL3_Init(window);
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
