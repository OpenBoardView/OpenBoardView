#include "ImGuiRendererSDL.h"

#include <sstream>
#include <vector>

#ifdef ENABLE_GLES2
#include <glad/gles2.h>
#else
#include <glad/gl.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "filesystem_impl.h"
#include "backends/imgui_impl_sdl2.h"
#include "../utils.h"

float ImGuiRendererSDL::getDisplayScale() {
	// Scaling from platform display DPI is only supported on Windows for now as it is broken and inconsistent on X11/XWayland/Wayland
#ifdef _WIN32
	return ImGui_ImplSDL2_GetContentScaleForDisplay(0);
#else
	return 1.0f;
#endif
}

ImGuiRendererSDL::ImGuiRendererSDL() {
	// SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	// SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	// SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
}

ImGuiRendererSDL::~ImGuiRendererSDL() {
}

std::string ImGuiRendererSDL::name() {
	return "ImGuiRendererSDL";
}

void ImGuiRendererSDL::setGLVersion() {
	SDL_GL_ResetAttributes();
}

bool ImGuiRendererSDL::init() {
	SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "Initializing %s", this->name().c_str());

	SDL_GL_LoadLibrary(nullptr);

	this->setGLVersion();

	// Setup window
	uint32_t window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
#if defined(_WIN32) || defined(__APPLE__)
	window_flags |= SDL_WINDOW_ALLOW_HIGHDPI;
#endif
	window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, window_flags);

	if (window == nullptr) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s: Failed to create the sdlWindow: %s\n", this->name().c_str(), SDL_GetError());
		shutdown();
		return false;
	}

	glcontext = SDL_GL_CreateContext(window);

	if (glcontext == nullptr) {
		SDL_LogError(
		    SDL_LOG_CATEGORY_ERROR, "%s: Failed to initialize OpenGL context: %s\n", this->name().c_str(), SDL_GetError());
		shutdown();
		return false;
	}

#ifdef ENABLE_GLES2
	this->version = gladLoadGLES2(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress));
#else
	this->version = gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress));
#endif
	if (!version) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s: glad failed to load OpenGL\n", this->name().c_str());
		shutdown();
		return false;
	}

	/* Query OpenGL device information */
	const GLubyte *strrenderer = glGetString(GL_RENDERER);
	const GLubyte *vendor      = glGetString(GL_VENDOR);
	const GLubyte *glversion   = glGetString(GL_VERSION);
	const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glMaxTextureSize);

	std::stringstream ss;
	ss << "\n-------------------------------------------------------------\n";
	ss << "GL Vendor         : " << vendor;
	ss << "\nGL GLRenderer    : " << strrenderer;
	ss << "\nGL Version       : " << glversion;
	ss << "\nGLSL Version     : " << glslVersion;
	ss << "\nMax texture size : " << glMaxTextureSize;
	ss << "\n-------------------------------------------------------------\n";
	SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "%s", ss.str().c_str());

	// Use Vsync
	if (SDL_GL_SetSwapInterval(1) < 0)
		SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "%s: Unable to enable VSync: %s\n", this->name().c_str(), SDL_GetError());

	// check if we got the correct OpenGL context version
	if (!this->checkGLVersion(this->version)) {
		shutdown();
		return false;
	}

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
	if (ImGui::GetIO().BackendPlatformUserData != nullptr) {
		ImGui_ImplSDL2_Shutdown();
	}

	if (glcontext != nullptr) {
		SDL_GL_DeleteContext(glcontext);
	}
	glcontext = nullptr;

	if (window != nullptr) {
		SDL_DestroyWindow(window);
	}
	window = nullptr;

	SDL_GL_UnloadLibrary();
}

void ImGuiRendererSDL::deleteTexture(ImTextureID tex) {
	GLuint texID = (size_t)tex;
	glDeleteTextures(1, &texID);
}

void ImGuiRendererSDL::toggleFullScreen() {
	if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) {
		SDL_SetWindowFullscreen(window, 0);
	} else {
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
	}
}

SDL_Window *ImGuiRendererSDL::getWindow() {
	return window;
}

std::string ImGuiRendererSDL::loadTextureFromFile(const filesystem::path &filepath, ImTextureID* out_texture, int* out_width, int* out_height) {
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

	ImTextureID image_texture;
	error_msg = createTexture(&image_texture, image_data, image_width, image_height, 1);

	stbi_image_free(image_data);

	if (!error_msg.empty()) {
		return filepath.string() + ": " + error_msg;
	}

	if (!image_texture) {
		return filepath.string() + ": could not create texture";
	}

	*out_texture = image_texture;
	*out_width = image_width;
	*out_height = image_height;

	return {};
}
