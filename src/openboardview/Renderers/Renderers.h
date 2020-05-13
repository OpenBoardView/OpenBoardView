#ifndef _RENDERERS_H_
#define _RENDERERS_H_

#include "ImGuiRendererSDLGL3.h"
#include "ImGuiRendererSDLGL1.h"

#include <memory>

namespace Renderers {
	enum class Renderer {
		OPENGL1 = 1, // Backward compatibility, adapt operator++ definition if changing this
		OPENGL3,
		DEFAULT // Boundary check, needs to stay at the end
	};

	const Renderer Preferred = Renderer::OPENGL3;

	Renderer &operator++(Renderer& r);

	Renderer get(int n);

	extern std::unique_ptr<ImGuiRendererSDL> current;

	std::unique_ptr<ImGuiRendererSDL> newInstance(Renderer r, SDL_Window *window);
	bool initBestRenderer(Renderer preferred, SDL_Window *window);
}

#endif
