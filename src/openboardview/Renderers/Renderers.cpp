#include "Renderers.h"

#include <SDL.h>

#include <iostream>

namespace Renderers {
	Renderer &operator++(Renderer& r) {
		return r = (r == Renderer::DEFAULT) ? static_cast<Renderer>(1 /*first renderer in Renderer enum class*/) : static_cast<Renderer>(static_cast<int>(r)+1);
	}

	Renderer get(int n) {
		if (n > static_cast<int>(Renderer::DEFAULT)) {
			std::cerr << "Unknown renderer specified: " << n << std::endl; // this is called before SDL is initalized when parsing parameters so we can't use SDL_Log
			return Renderer::DEFAULT;
		}
		return static_cast<Renderer>(n);
	}

	std::unique_ptr<ImGuiRendererSDL> newInstance(Renderer r, SDL_Window *window) {
		switch (r) {
#ifdef ENABLE_GL1
			case Renderer::OPENGL1:
				return std::unique_ptr<ImGuiRendererSDL>(new ImGuiRendererSDLGL1(window));
#endif
#ifdef ENABLE_GL3
			case Renderer::OPENGL3:
				return std::unique_ptr<ImGuiRendererSDL>(new ImGuiRendererSDLGL3(window));
#endif
			case Renderer::DEFAULT: // skip this one
				return std::unique_ptr<ImGuiRendererSDL>{};
			default:
				SDL_LogError(SDL_LOG_CATEGORY_RENDER, "%s", "No or unknown renderer specified.");
				return std::unique_ptr<ImGuiRendererSDL>{};
		}
	}

	std::unique_ptr<ImGuiRendererSDL> initBestRenderer(Renderer preferred, SDL_Window *window) {
		Renderer current = preferred;
		std::unique_ptr<ImGuiRendererSDL> rendererInstance;
		bool initialized = false;
		do {
			rendererInstance = newInstance(current, window);
			initialized = rendererInstance && rendererInstance->init() && rendererInstance->checkGLVersion(); // initialize and check if we got the correct OpenGL context version
		} while (++current != preferred && !initialized); // stop if we looped over or if it initalized successfully

		if (current == preferred && !initialized) { // we looped over and it didn't initialize
			rendererInstance.reset();
		}

		return rendererInstance;
	}
}
