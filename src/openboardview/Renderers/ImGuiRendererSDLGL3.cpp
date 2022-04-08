#include "ImGuiRendererSDLGL3.h"

#include "backends/imgui_impl_opengl3.h"

std::string ImGuiRendererSDLGL3::name() {
    return "ImGuiRendererSDLGL3";
}

bool ImGuiRendererSDLGL3::checkGLVersion() {
	if (GLVersion.major < 3 || (GLVersion.major == 3 && GLVersion.minor < 2)) {
		SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Minimal OpenGL version required is %d.%d. Got %d.%d.", 3, 2, GLVersion.major, GLVersion.minor);
		return false;
	}

	const std::string vendor{reinterpret_cast<const char *>(glGetString(GL_VENDOR))};
	const std::string strrenderer{reinterpret_cast<const char *>(glGetString(GL_RENDERER))};

	// Check if Tesla with nouveau, buggy with ImGui 1.87 GL3 so should fall back to GL1
	if (vendor == "nouveau"
		&& (!strrenderer.compare(0, 3, "NV5")
			|| !strrenderer.compare(0, 3, "NV8")
			|| !strrenderer.compare(0, 3, "NV9")
			|| !strrenderer.compare(0, 3, "NVA"))) {
		SDL_LogError(SDL_LOG_CATEGORY_RENDER, "OpenGL3 renderer not supported on %s %s", vendor.c_str(), strrenderer.c_str());
		return false;
	}

	return true;
}

void ImGuiRendererSDLGL3::setGLVersion() {
	ImGuiRendererSDL::setGLVersion();
	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GLES 2.0 + GLSL 100
	glsl_version = "#version 100";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
	// GLES 3.0 + GLSL 300 ES
	glsl_version = "#version 300 es";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif __APPLE__
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
#endif
	// GL 3.2 Core + GLSL 150
	glsl_version = "#version 150";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
}

bool ImGuiRendererSDLGL3::init() {
	return ImGuiRendererSDL::init() && ImGui_ImplOpenGL3_Init(glsl_version.c_str());
}

void ImGuiRendererSDLGL3::initFrame() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGuiRendererSDL::initFrame();
}

void ImGuiRendererSDLGL3::renderDrawData() {
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiRendererSDLGL3::shutdown() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGuiRendererSDL::shutdown();
}
