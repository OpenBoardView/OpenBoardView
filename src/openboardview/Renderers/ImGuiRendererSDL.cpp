#include "ImGuiRendererSDL.h"

#include <sstream>
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "backends/imgui_impl_sdl2.h"

#include "utils.h"

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

	//Use Vsync
	if (SDL_GL_SetSwapInterval(1) < 0)
		SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "%s: Unable to enable VSync: %s\n", this->name().c_str(), SDL_GetError());

	// check if we got the correct OpenGL context version
	if (!checkGLVersion())
		return false;

	ImGui_ImplSDL2_InitForOpenGL(window, glcontext);

	return true;
}

void ImGuiRendererSDL::processEvent(SDL_Event &event) {
	ImGui_ImplSDL2_ProcessEvent(&event);
}

void ImGuiRendererSDL::initFrame() {
	ImGui_ImplSDL2_NewFrame();
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

std::string ImGuiRendererSDL::loadTextureFromFile(const filesystem::path &filepath, GLuint* out_texture, int* out_width, int* out_height)
{
	// Load from file
	int image_width = 0;
	int image_height = 0;

	std::string error_msg;
	auto buf = file_as_buffer(filepath, error_msg);
	if (buf.empty() || !error_msg.empty()) {
		return filepath.string() + ": " + error_msg;
	}
	unsigned char* image_data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(buf.data()), buf.size(), &image_width, &image_height, NULL, 4);

	if (image_data == nullptr) {
		return "Could not load image from " + filepath.string() + ": " + stbi_failure_reason();
	}

	int glMaxTextureSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glMaxTextureSize);
	if (image_width > glMaxTextureSize) {
		return filepath.string() + ": width of " + std::to_string(image_width) + "px is too large for this GPU. Maximum allowed: " + std::to_string(glMaxTextureSize);
	}

	if (image_height > glMaxTextureSize) {
		return filepath.string() + ": height of " + std::to_string(image_width) + "px is too large for this GPU. Maximum allowed: " + std::to_string(glMaxTextureSize);
	}

	// Create a OpenGL texture identifier
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Upload pixels into texture
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

	GLenum code = glGetError();
	if (code == GL_OUT_OF_MEMORY) {
		return filepath.string() + ": image too large to fit in the GPU memory.";
	}
	if (code != GL_NO_ERROR) {
		return filepath.string() + ": error " + std::to_string(code) + " when loading the image into GPU memory.";
	}

	stbi_image_free(image_data);

	*out_texture = image_texture;
	*out_width = image_width;
	*out_height = image_height;

	return {};
}
