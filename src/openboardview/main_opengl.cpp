/**
 * OpenBoardView
 *
 * Copyright inflex 2016 (Paul Daniels)
 * Copyright chloridite 2016
 *
 * https://github.com/OpenBoardView/OpenBoardView
 *
 */

#include "platform.h" // Should be kept first
#include "utils.h"
#include "version.h"

#include "BoardView.h"
#include "history.h"

#include "FileFormats/FZFile.h"
#include "confparse.h"
#include "resource.h"
#ifdef _MSC_VER
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include <deque>
#include <memory>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

// Rendering stuff
#include "Renderers/Renderers.h"

struct globals {
	char *input_file;
	char *config_file;
	bool slowCPU;
	int width;
	int height;
	int dpi;
	double font_size;
	bool debug;
	Renderers::Renderer renderer;

	globals() {
		this->input_file  = NULL;
		this->config_file = NULL;
		this->slowCPU     = false;
		this->width       = 0;
		this->height      = 0;
		this->dpi         = 0;
		this->font_size   = 0.0f;
		this->debug       = false;
		this->renderer    = Renderers::Renderer::DEFAULT;
	}
};

static SDL_Window *window      = nullptr;

char help[] =
    " [-h] [-V] [-l] [-c <config file>] [-i <intput file>] [-x <width>] [-y <height>] [-z <fontsize>] [-p <dpi>] [-r <renderer>] [-d]\n\
	-h : This help\n\
	-V : Version information\n\
	-l : slow CPU mode, disables AA and other items to try provide more FPS\n\
	-c <config file> : alternative configuration file (default is ~/.config/" OBV_NAME
    "/obv.conf)\n\
	-i <input file> : board file to load\n\
	-x <width> : Set window width\n\
	-y <height> : Set window height\n\
	-z <pixels> : Set font size\n\
	-p <dpi> : Set the dpi\n\
	-r <renderer> : Set the renderer [ OPENGL1 = 1; OPENGL3 = 2; OPENGLES2 = 3 ]\n\
	-d : Debug mode\n\
";

int parse_parameters(int argc, char **argv, struct globals *g) {
	int param;

	/*
	 * When we're using file-associations, the OS usually just
	 * passes the filename to be loaded as the single initial
	 * parameter, so in this special case situation we see if the
	 * single param is a valid file, and try load it.
	 */
	if (argc == 2) {
		if (access(argv[1], F_OK) != -1) {
			g->input_file = argv[1];
			return 0;
		}
	}

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

		if (strcmp(p, "-c") == 0) {
			param++;
			if ((param < argc)&&(argv[param][0] != '-')) {
				g->config_file = argv[param];
			} else {
				fprintf(stderr, "Not enough paramters for -c <config>\n\n%s %s", argv[0], help );
				exit(1);
			}

		} else if (strcmp(p, "-i") == 0) {
			param++;
			if ((param < argc)&&(argv[param][0] != '-')) {
				g->input_file = argv[param];
			} else {
				fprintf(stderr, "Not enough paramters for -i <input file>\n\n%s %s", argv[0], help );
				exit(1);
			}

		} else if (strcmp(p, "-x") == 0) {
			param++;
			if ((param < argc)&&(argv[param][0] != '-')) {
				g->width = strtol(argv[param], NULL, 10);
			} else {
				fprintf(stderr, "Not enough paramters for -x <window width>\n\n%s %s", argv[0], help );
				exit(1);
			}

		} else if (strcmp(p, "-y") == 0) {
			param++;
			if ((param < argc)&&(argv[param][0] != '-')) {
				g->height = strtol(argv[param], NULL, 10);
			} else {
				fprintf(stderr, "Not enough paramters for -y <window height>\n\n%s %s", argv[0], help );
				exit(1);
			}

		} else if (strcmp(p, "-z") == 0) {
			param++;
			if ((param < argc)&&(argv[param][0] != '-')) {
				g->font_size = strtof(argv[param], NULL);
			} else {
				fprintf(stderr, "Not enough paramters for -z <font size>\n\n%s %s", argv[0], help );
				exit(1);
			}

		} else if (strcmp(p, "-p") == 0) {
			param++;
			if ((param < argc)&&(argv[param][0] != '-')) {
				g->dpi = strtof(argv[param], NULL);
			} else {
				fprintf(stderr, "Not enough paramters for -p <dpi>\n\n%s %s", argv[0], help );
				exit(1);
			}

		} else if (strcmp(p, "-r") == 0) {
			param++;
			if ((param < argc)&&(argv[param][0] != '-')) {
				g->renderer = Renderers::get(atoi(argv[param]));
			} else {
				fprintf(stderr, "Not enough paramters for -r <render engine>\n\n%s %s", argv[0], help );
				exit(1);
			}

		} else if (strcmp(p, "-l") == 0) {
			g->slowCPU = true;

		} else if (strcmp(p, "-d") == 0) {
			g->debug = true;

		} else {
			fprintf(stderr, "Unknown parameter '%s'\n\n%s %s", p, argv[0], help);
			exit(1);
		}
	}

	return 0;
}

void cleanupAndExit(int c) {
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

	if (g.renderer == Renderers::Renderer::DEFAULT) {
		g.renderer = Renderers::get(app.obvconfig.ParseInt("renderer", static_cast<int>(Renderers::Preferred)));
	}

	// Setup window
	SDL_DisplayMode current;
	SDL_GetCurrentDisplayMode(0, &current);
	window = SDL_CreateWindow(
	    OBV_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, g.width, g.height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (window == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create the sdlWindow: %s\n", SDL_GetError());
		cleanupAndExit(1);
	}

	// Setup renderer
	std::unique_ptr<ImGuiRendererSDL> renderer = Renderers::initBestRenderer(g.renderer, window);
	if (!renderer) {
		SDL_LogError(SDL_LOG_CATEGORY_RENDER, "%s", "No renderer not available. Exiting.");
		cleanupAndExit(1);
	}

	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

	// SDL disables screen saver by default which doesn't make sense for us.
	SDL_EnableScreenSaver();

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

	// Preset some workable sizes
	app.m_board_surface.x = g.width;
	app.m_board_surface.y = g.height;
	if (app.showInfoPanel) app.m_board_surface.x -= app.m_info_surface.x;

	if (g.font_size == 0.0f) g.font_size = app.obvconfig.ParseDouble("fontSize", 20.0f);
	g.font_size                          = (g.font_size * app.dpi) / 100;

	{
		ImGuiStyle &style = ImGui::GetStyle();
		style.ScrollbarSize *= app.dpi / 100;
	}

	// Font selection
	std::deque<std::string> fontList(
	    {"Liberation Sans", "DejaVu Sans", "Arial", "Helvetica", ""}); // Empty string = use system default font
	std::string customFont(app.obvconfig.ParseStr("fontName", ""));

	if (!customFont.empty()) fontList.push_front(customFont);

	for (const auto &name : fontList) {
		app.obvconfig.WriteStr("fontName", name.c_str());
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
		if (fontpath.empty()) continue;        // Font not found
		if (check_fileext(fontpath, ".ttf")) { // ImGui handles only TrueType fonts so exclude anything which has a different ext
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
			renderer->processEvent(event);

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

		// Prepare frame
		renderer->initFrame();

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
			snprintf(scratch, sizeof(scratch), "%s - %s", OBV_NAME, app.fhistory.history[0]);
			SDL_SetWindowTitle(window, scratch);
			app.history_file_has_changed = 0;
		}

		// Render frame
		renderer->renderFrame(clear_color);
	}

	// Cleanup
 	renderer->shutdown();

	cleanupAndExit(0);
	return 0;
}
