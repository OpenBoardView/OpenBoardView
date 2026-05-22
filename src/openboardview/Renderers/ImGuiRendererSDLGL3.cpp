#include "ImGuiRendererSDLGL3.h"


#ifdef ENABLE_GLES2
#include <glad/gles2.h>
#else
#include <glad/gl.h>
#endif
#include "backends/imgui_impl_opengl3.h"

std::string ImGuiRendererSDLGL3::name() {
	return "ImGuiRendererSDLGL3";
}

bool ImGuiRendererSDLGL3::checkGLVersion(int version) {
	int major = GLAD_VERSION_MAJOR(version);
	int minor = GLAD_VERSION_MINOR(version);

	if (major < GL_VERSION_MAJOR || (major == GL_VERSION_MAJOR && minor < GL_VERSION_MINOR)) {
		SDL_LogError(SDL_LOG_CATEGORY_RENDER,
		             "Minimal %s version required is %d.%d. Got %d.%d.",
#if defined(IMGUI_IMPL_OPENGL_ES2) || defined(IMGUI_IMPL_OPENGL_ES3)
					 "OpenGLES",
#else
					 "OpenGL",
#endif
		             GL_VERSION_MAJOR,
		             GL_VERSION_MINOR,
		             major,
		             minor);
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
#elif defined(IMGUI_IMPL_OPENGL_ES3)
	// GLES 3.0 + GLSL 300 ES
	glsl_version = "#version 300 es";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
#if __APPLE__
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
#endif
	// GL 3.2 Core + GLSL 150
	glsl_version = "#version 150";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, GL_VERSION_MAJOR);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, GL_VERSION_MINOR);
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
	if (ImGui::GetIO().BackendRendererUserData != nullptr) {
		ImGui_ImplOpenGL3_Shutdown();
	}
	ImGuiRendererSDL::shutdown();
}

std::string ImGuiRendererSDLGL3::createTexture(ImTextureID* out_texture, uint8_t* data, int w, int h, char fmt) {
	GLuint tex;
	GLenum format;

	if (w > glMaxTextureSize) {
		return "width of " + std::to_string(w) + "px is too large for this GPU. Maximum allowed: " + std::to_string(glMaxTextureSize);
	}

	if (h > glMaxTextureSize) {
		return "height of " + std::to_string(h) + "px is too large for this GPU. Maximum allowed: " + std::to_string(glMaxTextureSize);
	}

	switch (fmt) {
		default:
		case 0:
#ifdef ENABLE_GLES2
			format = GL_BGRA_EXT;
#else
			format = GL_BGRA;
#endif
			break;
		case 1:
			format = GL_RGBA;
			break;
		case 2:
			format = GL_RGB;
			break;
	}

#ifdef ENABLE_GLES2
	GLint internalformat = format;
#else
	GLint internalformat = GL_RGBA;
#endif

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, w, h, 0, format, GL_UNSIGNED_BYTE, data);

	GLenum code = glGetError();
	if (code == GL_OUT_OF_MEMORY) {
		return "image too large to fit in the GPU memory.";
	}
	if (code != GL_NO_ERROR) {
		return "error " + std::to_string(code) + " when loading the image into GPU memory.";
	}

	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	*out_texture = tex;

	return {};
}
