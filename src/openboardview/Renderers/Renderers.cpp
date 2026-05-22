#include "Renderers.h"

#include <SDL.h>

#include <iostream>

namespace Renderers {
	std::unique_ptr<ImGuiRendererSDL> current;

	Renderer &operator++(Renderer &r) {
		return r = (r == Renderer::DEFAULT) ? static_cast<Renderer>(1 /*first renderer in Renderer enum class*/)
											: static_cast<Renderer>(static_cast<int>(r) + 1);
	}

	Renderer get(int n) {
		if (n > static_cast<int>(Renderer::DEFAULT)) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unknown renderer specified: %d\n", n);
			return Renderer::DEFAULT;
		}
		return static_cast<Renderer>(n);
	}

	std::unique_ptr<ImGuiRendererSDL> newInstance(Renderer r) {
		switch (r) {
#ifdef ENABLE_GL1
			case Renderer::OPENGL1:
				return std::unique_ptr<ImGuiRendererSDL>(new ImGuiRendererSDLGL1());
#endif
#ifdef ENABLE_GL3
			case Renderer::OPENGL3:
				return std::unique_ptr<ImGuiRendererSDL>(new ImGuiRendererSDLGL3());
#endif
			case Renderer::DEFAULT: // skip this one
				return std::unique_ptr<ImGuiRendererSDL>{};
			default:
				SDL_LogError(SDL_LOG_CATEGORY_RENDER, "%s", "No or unknown renderer specified.");
				return std::unique_ptr<ImGuiRendererSDL>{};
		}
	}

	bool initBestRenderer(Renderer preferred) {
		Renderer tryrenderer = preferred;
		std::unique_ptr<ImGuiRendererSDL> rendererInstance;
		bool initialized = false;
		do {
			rendererInstance = newInstance(tryrenderer);
			initialized = rendererInstance && rendererInstance->init();
		} while (++tryrenderer != preferred && !initialized); // stop if we looped over or if it initalized successfully

		if (tryrenderer == preferred && !initialized) { // we looped over and it didn't initialize
			rendererInstance.reset();
		} else {
			current = std::move(rendererInstance);
		}
		return initialized;
	}
} // namespace Renderers
