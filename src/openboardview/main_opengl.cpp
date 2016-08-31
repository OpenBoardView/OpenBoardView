/**
 * Open[Flex] Board View
 *
 * Copyright chloridite 2016
 * Copyright inflex 2016 (Paul Daniels)
 *
 * Git Fork: https://github.com/inflex/OpenBoardView
 *
 */

#include "platform.h" // Should be kept first

#include "BoardView.h"
#include "history.h"

#include "FZFile.h"
#include "confparse.h"
#include "resource.h"
#ifdef _MSC_VER
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include <sstream>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

// Rendering stuff
#ifdef ENABLE_GL1
#include "Renderers/imgui_impl_sdl.h"
#endif
#ifdef ENABLE_GL3
#include "Renderers/imgui_impl_sdl_gl3.h"
#endif
#ifdef ENABLE_GLES2
#include "Renderers/imgui_impl_sdl_gles2.h"
#endif
#include <glad/glad.h>

enum class Renderer { DEFAULT, OPENGL1, OPENGL3, OPENGLES2 };

struct globals {
	char *input_file;
	char *config_file;
	bool slowCPU;
	int width;
	int height;
	int dpi;
	double font_size;
	bool debug;
	Renderer renderer;

	globals() {
		this->input_file  = NULL;
		this->config_file = NULL;
		this->slowCPU     = false;
		this->width       = 0;
		this->height      = 0;
		this->dpi         = 0;
		this->font_size   = 0.0f;
		this->debug       = false;
		this->renderer    = Renderer::DEFAULT;
	}
};

static SDL_GLContext glcontext = NULL;
static SDL_Window *window      = nullptr;

char help[] =
    " [-h] [-V] [-l] [-c <config file>] [-i <intput file>] [-x <width>] [-y <height>] [-z <fontsize>] [-p <dpi>] [-r <renderer>] [-d]\n\
	-h : This help\n\
	-V : Version information\n\
	-l : slow CPU mode, disables AA and other items to try provide more FPS\n\
	-c <config file> : alternative configuration file (default is ~/.config/openboardview/obv.conf)\n\
	-i <input file> : board file to load\n\
	-x <width> : Set window width\n\
	-y <height> : Set window height\n\
	-z <pixels> : Set font size\n\
	-p <dpi> : Set the dpi\n\
	-r <renderer> : Set the renderer [ OPENGL1 = 0; OPENGL3 = 1; OPENGLES2 = 2 ]\n\
	-d : Debug mode\n\
";

int parse_parameters(int argc, char **argv, struct globals *g) {
	int param;

	/**
	 * Decode the input parameters.
	 * Yes, I know, I should do this using gnu params etc.
	 */
	for (param = 1; param < argc; param++) {
		char *p = argv[param];

		if (strcmp(p, "-h") == 0) {
			fprintf(stdout, "%s %s", argv[0], help);
			exit(0);
		}

		if (strcmp(p, "-V") == 0) {
			fprintf(stdout, "OFBV-BUILD: %s %s\n", OBV_BUILD, __TIMESTAMP__);
			exit(0);
		}

		// Configuration file (alternative)
		if (strcmp(p, "-c") == 0) {
			param++;
			if (param < argc) {
				g->config_file = argv[param];
			} else {
				fprintf(stderr, "Not enough paramters\n");
				exit(1);
			}

		} else if (strcmp(p, "-i") == 0) {
			param++;
			if (param < argc) {
				g->input_file = argv[param];
			} else {
				fprintf(stderr, "Not enough parameters\n");
			}

		} else if (strcmp(p, "-x") == 0) {
			param++;
			if (param < argc) {
				g->width = strtol(argv[param], NULL, 10);
			} else {
				fprintf(stderr, "Not enough parameters\n");
			}

		} else if (strcmp(p, "-y") == 0) {
			param++;
			if (param < argc) {
				g->height = strtol(argv[param], NULL, 10);
			} else {
				fprintf(stderr, "Not enough parameters\n");
			}

		} else if (strcmp(p, "-z") == 0) {
			param++;
			if (param < argc) {
				g->font_size = strtof(argv[param], NULL);
			} else {
				fprintf(stderr, "Not enough parameters\n");
			}

		} else if (strcmp(p, "-p") == 0) {
			param++;
			if (param < argc) {
				g->dpi = strtof(argv[param], NULL);
			} else {
				fprintf(stderr, "Not enough parameters\n");
			}

		} else if (strcmp(p, "-r") == 0) {
			param++;
			if (param < argc) {
				switch (atoi(argv[param])) {
					case 1: g->renderer = Renderer::OPENGL1; break;
					case 2: g->renderer = Renderer::OPENGL3; break;
					case 3: g->renderer = Renderer::OPENGLES2; break;
					default: fprintf(stderr, "Unknown renderer. Using default\n");
				}
			} else {
				fprintf(stderr, "Not enough parameters\n");
			}

		} else if (strcmp(p, "-l") == 0) {
			g->slowCPU = true;

		} else if (strcmp(p, "-d") == 0) {
			g->debug = true;

			/*
			 * for extended parameters, nothing yet required
			 *
			 *
		  } else if (strncmp(p,"--", 2) == 0) {

			 */
		}
	}

	return 0;
}

void cleanupAndExit(int c) {
	if (glcontext) SDL_GL_DeleteContext(glcontext);
	if (window) SDL_DestroyWindow(window);
	SDL_Quit();
	exit(c);
}

int main(int argc, char **argv) {
	uint8_t sleepout;
	std::string configDir;
	globals g; // because some things we have to store *before* we load the config file in BoardView app.obvconf
	BoardView app{};

	/*
	 * Parse the parameters first up, store the results in the global struct.
	 *
	 * This does mean a little more redundancy between the OS builds but not
	 * as bad as it was before.
	 *
	 */
	parse_parameters(argc, argv, &g);

	app.debug = g.debug;

	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

	// Load the configuration file
	configDir = get_user_dir(UserDir::Config);
	if (!configDir.empty()) app.obvconfig.Load(configDir + "obv.conf");

	// Load file history
	std::string dataDir = get_user_dir(UserDir::Data);
	if (!dataDir.empty()) {
		app.fhistory.Set_filename(dataDir + "obv.history");
		app.fhistory.Load();
	}

	// If we've chosen to override the normally found config.
	if (g.config_file) app.obvconfig.Load(g.config_file);

	// Apply the slowCPU flag if required.
	app.slowCPU = g.slowCPU;

	if (g.width == 0) g.width   = app.obvconfig.ParseInt("windowX", 1100);
	if (g.height == 0) g.height = app.obvconfig.ParseInt("windowY", 700);

	if (g.renderer == Renderer::DEFAULT) {
		switch (app.obvconfig.ParseInt("renderer", 2)) {
			case 1: g.renderer = Renderer::OPENGL1; break;
			case 2: g.renderer = Renderer::OPENGL3; break;
			case 3: g.renderer = Renderer::OPENGLES2; break;
		}
	}
// Setup window

#ifdef ENABLE_GL1
	if (g.renderer == Renderer::OPENGL1) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	}
#endif
#ifdef ENABLE_GL3
	if (g.renderer == Renderer::OPENGL3) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	}
#endif
#ifdef ENABLE_GLES2
	if (g.renderer == Renderer::OPENGLES2) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	}
#endif
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	SDL_GL_LoadLibrary(NULL);

	SDL_DisplayMode current;
	SDL_GetCurrentDisplayMode(0, &current);
	window = SDL_CreateWindow("Openflex Boardviewer",
	                          SDL_WINDOWPOS_CENTERED,
	                          SDL_WINDOWPOS_CENTERED,
	                          g.width,
	                          g.height,
	                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (window == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create the sdlWindow: %s\n", SDL_GetError());
		cleanupAndExit(1);
	}

	glcontext = SDL_GL_CreateContext(window);
	if (glcontext == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to initialize OpenGL context: %s\n", SDL_GetError());
		cleanupAndExit(1);
	}

	if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "glad failed to load OpenGL\n");
		cleanupAndExit(1);
	}

	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

	bool initialized = false;
// Setup ImGui binding
#ifdef ENABLE_GL1
	if (g.renderer == Renderer::OPENGL1) {
		initialized = ImGui_ImplSdl_Init(window);
	}
#endif
#ifdef ENABLE_GL3
	if (g.renderer == Renderer::OPENGL3) {
		initialized = ImGui_ImplSdlGL3_Init(window);
	}
#endif
#ifdef ENABLE_GLES2
	if (g.renderer == Renderer::OPENGLES2) {
		initialized = ImGui_ImplSdlGLES2_Init(window);
	}
#endif

#if defined(ENABLE_GL1) || defined(ENABLE_GL3) || defined(ENABLE_GLES2)
	if (!initialized) {
		SDL_LogError(SDL_LOG_CATEGORY_RENDER, "%s", "Selected renderer is not available. Exiting.");
		cleanupAndExit(1);
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
#else
	SDL_LogError(SDL_LOG_CATEGORY_RENDER, "%s", "No renderer was built in the application. Exiting.");
	cleanupAndExit(1);
#endif

	ImGuiIO &io    = ImGui::GetIO();
	io.IniFilename = NULL;
	//	io.Fonts->AddFontDefault();

	// Main loop
	bool done             = false;
	bool preload_required = false;

	// Set the dpi, if we've not set any parameters it'll be 0 which
	// will make the ConfigParse load and set the right dpi.
	app.dpi = g.dpi;

	// Now that the configuration file is loaded in to BoardView, parse its settings.
	app.ConfigParse();

	if (g.font_size == 0.0f) g.font_size = app.obvconfig.ParseDouble("fontSize", 20.0f);
	g.font_size                          = (g.font_size * app.dpi) / 100;

	for (auto name : {"Liberation Sans", "DejaVu Sans", "Arial", "Helvetica", ""}) { // Empty string = use system default font
#ifdef _WIN32
		ImFontConfig font_cfg{};
		font_cfg.FontDataOwnedByAtlas = false;
		const std::vector<char> ttf   = load_font(name);
		if (!ttf.empty()) {
			io.Fonts->AddFontFromMemoryTTF(
			    const_cast<void *>(reinterpret_cast<const void *>(ttf.data())), ttf.size(), g.font_size, &font_cfg);
			break;
		}
#else
		const std::string fontpath = get_font_path(name);
		if (fontpath.empty()) continue; // Font not found
		auto extpos = fontpath.rfind('.');
		if (extpos == std::string::npos) continue; // No extension in filename
		std::string ext = fontpath.substr(extpos);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower); // make ext lowercase
		if (ext == ".ttf") { // ImGui handles only TrueType fonts so exclude anything which has a different ext
			io.Fonts->AddFontFromFileTTF(fontpath.c_str(), g.font_size);
			break;
		}
#endif
	}

	// ImVec4 clear_color = ImColor(20, 20, 30);
	ImVec4 clear_color = ImColor(app.m_colors.backgroundColor);

	/*
	 * If we've asked to load a file from the command line
	 * then this is where we stage it to be loaded directly
	 * in to OBV
	 */
	if (g.input_file) {
		struct stat buffer;
		if ((stat(g.input_file, &buffer) == 0)) {
			preload_required = true;
		}
	}

	/*
	 * The sleepout var keeps track of how many iterations of the main loop
	 * are left, without an event having happened before OBV will start to sleep
	 * and continue without redrawing the page.
	 *
	 * The reason we don't just sleep immediately after a non-event is because
	 * sometimes there are internal things that still need to be done on the next
	 * render (such as responding to a mouse click
	 *
	 * For now we've got this set to 3 frames which seems to work okay with OBV.
	 * If you find some things aren't working properly without you having to move
	 * the mouse or 'waking up' OBV then increase to 5 or more.
	 */
	sleepout = 3;
	while (!done) {

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			sleepout = 3;
#ifdef ENABLE_GL1
			if (g.renderer == Renderer::OPENGL1) ImGui_ImplSdl_ProcessEvent(&event);
#endif
#ifdef ENABLE_GL3
			if (g.renderer == Renderer::OPENGL3) ImGui_ImplSdlGL3_ProcessEvent(&event);
#endif
#ifdef ENABLE_GLES2
			if (g.renderer == Renderer::OPENGLES2) ImGui_ImplSdlGLES2_ProcessEvent(&event);
#endif

			if (event.type == SDL_DROPFILE) {
				// Validate the file before replacing the current one, not that we
				// should have to, but always better to be safe
				struct stat buffer;
				if (stat(event.drop.file, &buffer) == 0) {
					app.LoadFile(strdup(event.drop.file));
				}
			}

			if (event.type == SDL_QUIT) done = true;
		}

		if (app.reloadConfig) {
			app.reloadConfig = false;
			app.obvconfig.Load(configDir + "obv.conf");
			app.ConfigParse();
			clear_color = ImColor(app.m_colors.backgroundColor);
		}

		if (!(sleepout--)) {
#ifdef _WIN32
			Sleep(50);
#else
			usleep(50000);
#endif
			sleepout = 0;
			continue;
		} // puts OBV to sleep if nothing is happening.

#ifdef ENABLE_GL1
		if (g.renderer == Renderer::OPENGL1) ImGui_ImplSdl_NewFrame(window);
#endif
#ifdef ENABLE_GL3
		if (g.renderer == Renderer::OPENGL3) ImGui_ImplSdlGL3_NewFrame(window);
#endif
#ifdef ENABLE_GLES2
		if (g.renderer == Renderer::OPENGLES2) ImGui_ImplSdlGLES2_NewFrame();
#endif

		// If we have a board to view being passed from command line, then "inject"
		// it here.
		if (preload_required) {
			app.LoadFile(strdup(g.input_file));
			preload_required = false;
		}

		app.Update();
		if (app.m_wantsQuit) {
			SDL_Event sdlevent;
			sdlevent.type = SDL_QUIT;
			SDL_PushEvent(&sdlevent);
		}

		// Update the title of the SDL app if the board filename has changed. -
		// PLD20160618
		if (app.history_file_has_changed) {
			char scratch[1024];
			snprintf(scratch, sizeof(scratch), "Openflex Boardviewer - %s", app.fhistory.history[0]);
			SDL_SetWindowTitle(window, scratch);
			app.history_file_has_changed = 0;
		}

// Rendering
#if defined(ENABLE_GL1) || defined(ENABLE_GL3) || defined(ENABLE_GLES2)
		glViewport(0, 0, static_cast<int>(ImGui::GetIO().DisplaySize.x), static_cast<int>(ImGui::GetIO().DisplaySize.y));
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
#endif
		ImGui::Render();
		SDL_GL_SwapWindow(window);
	}

// Cleanup
#ifdef ENABLE_GL1
	if (g.renderer == Renderer::OPENGL1) ImGui_ImplSdl_Shutdown();
#endif
#ifdef ENABLE_GL3
	if (g.renderer == Renderer::OPENGL3) ImGui_ImplSdlGL3_Shutdown();
#endif
#ifdef ENABLE_GLES2
	if (g.renderer == Renderer::OPENGLES2) ImGui_ImplSdlGLES2_Shutdown();
#endif

	cleanupAndExit(0);
	return 0;
}
