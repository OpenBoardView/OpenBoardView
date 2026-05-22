#include "ImGuiRendererSDLGL1.h"

#include <glad/gl.h>
#include "backends/imgui_impl_opengl2.h"

std::string ImGuiRendererSDLGL1::name() {
	return "ImGuiRendererSDLGL1";
}

bool ImGuiRendererSDLGL1::checkGLVersion(int version) {
	int major = GLAD_VERSION_MAJOR(version);
	int minor = GLAD_VERSION_MINOR(version);

	if (major < 1 || (major == 1 && minor < 1)) {
		SDL_LogError(SDL_LOG_CATEGORY_RENDER,
		             "Minimal OpenGL version required is %d.%d. Got %d.%d.",
		             1,
		             1,
		             major,
		             minor);
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
	if (ImGui::GetIO().BackendRendererUserData != nullptr) {
		ImGui_ImplOpenGL2_Shutdown();
	}
	ImGuiRendererSDL::shutdown();
}

std::string ImGuiRendererSDLGL1::createTexture(ImTextureID* out_texture, uint8_t* data, int w, int h, char fmt) {
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
			format = GL_BGRA;
			break;
		case 1:
			format = GL_RGBA;
			break;
		case 2:
			format = GL_RGB;
			break;
	}

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	int major = GLAD_VERSION_MAJOR(version);
	int minor = GLAD_VERSION_MINOR(version);
	if (major == 2 || (major == 1 && minor >= 4)) {
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, format, GL_UNSIGNED_BYTE, data);

	GLenum code = glGetError();
	if (code == GL_OUT_OF_MEMORY) {
		return "image too large to fit in the GPU memory.";
	}
	if (code != GL_NO_ERROR) {
		return "error " + std::to_string(code) + " when loading the image into GPU memory.";
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	*out_texture = tex;

	return {};
}
