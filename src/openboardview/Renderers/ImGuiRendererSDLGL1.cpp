#include "ImGuiRendererSDLGL1.h"

#include "backends/imgui_impl_opengl2.h"

std::string ImGuiRendererSDLGL1::name() {
    return "ImGuiRendererSDLGL1";
}

bool ImGuiRendererSDLGL1::checkGLVersion() {
	if (GLVersion.major < 1 || (GLVersion.major == 1 && GLVersion.minor < 1)) {
		SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Minimal OpenGL version required is %d.%d. Got %d.%d.", 1, 1, GLVersion.major, GLVersion.minor);
		return false;
	}
	return true;
}

void ImGuiRendererSDLGL1::setGLVersion() {
	ImGuiRendererSDL::setGLVersion();
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
}

bool ImGuiRendererSDLGL1::init() {
	return ImGuiRendererSDL::init() && ImGui_ImplOpenGL2_Init();
}

void ImGuiRendererSDLGL1::initFrame() {
	ImGui_ImplOpenGL2_NewFrame();
	ImGuiRendererSDL::initFrame();
}

void ImGuiRendererSDLGL1::renderDrawData() {
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiRendererSDLGL1::shutdown() {
	ImGui_ImplOpenGL2_Shutdown();
	ImGuiRendererSDL::shutdown();
}
