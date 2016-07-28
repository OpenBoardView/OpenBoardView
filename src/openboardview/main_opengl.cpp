// ImGui - standalone example application for SDL2 + OpenGL
// If you are new to ImGui, see examples/README.txt and documentation at the top
// of imgui.cpp.

#include "BoardView.h"
#include "history.h"

#include "platform.h"
#include "FZFile.h"
#include "confparse.h"
#include "imgui_impl_sdl_gl3.h"
#include "resource.h"
#include <GL/gl3w.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct globals {
	char *input_file;
	char *config_file;
	bool slowCPU;
	int width;
	int height;
	double font_size;

	globals() {
		this->input_file  = NULL;
		this->config_file = NULL;
		this->slowCPU     = false;
		this->width       = 0;
		this->height      = 0;
		this->font_size   = 0.0f;
	}
};

char help[] =
    " [-h] [-l] [-c <config file>] [-i <intput file>]\n\
	-h : This help\n\
	-l : slow CPU mode, disables AA and other items to try provide more FPS\n\
	-c <config file> : alternative configuration file (default is ~/.config/openboardview/obv.conf)\n\
	-i <input file> : board file to load\n\
	-x <width> : Set window width\n\
	-y <height> : Set window height\n\
	-z <pixels> : Set font size\n\
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

		} else if (strcmp(p, "-l") == 0) {

			g->slowCPU = true;

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

int main(int argc, char **argv) {
	uint8_t sleepout;
	char s[1025];
	char *homepath;
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

	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

	/*
	 * *nix specific, usually we have a $HOME env var set and
	 * from that we can see if we have a ~/.config in which we
	 * can create our openboardview folder for storing what ever
	 * stuff we need (currently file-history and configuration file)
	 *
	 */
	homepath = getenv("HOME");
	if (homepath) {
		struct stat st;
		int sr;
		snprintf(s, sizeof(s), "%s/.config/openboardview", homepath);
		sr = stat(s, &st);
		if (sr == -1) {
			mkdir(s, S_IRWXU);
			sr = stat(s, &st);
		}

		/*
		 * Check to see if the path exists, if it does, create the full
		 * filenames and load up
		 */
		if ((sr == 0) && (S_ISDIR(st.st_mode))) {
			snprintf(s, sizeof(s), "%s/.config/openboardview/obv.conf", homepath);
			app.obvconfig.Load(s);

			snprintf(s, sizeof(s), "%s/.config/openboardview/obv.history", homepath);
			app.fhistory.Set_filename(s);
			app.fhistory.Load();
		}
	}

	// If we've chosen to override the normally found config.
	if (g.config_file) app.obvconfig.Load(g.config_file);

	// Apply the slowCPU flag if required.
	app.slowCPU = g.slowCPU;

	if (g.width == 0) g.width   = app.obvconfig.ParseInt("windowX", 800);
	if (g.height == 0) g.height = app.obvconfig.ParseInt("windowY", 600);

	// Setup window
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_DisplayMode current;
	SDL_GetCurrentDisplayMode(0, &current);
	SDL_Window *window = SDL_CreateWindow("OpenFlex Board Viewer",
	                                      SDL_WINDOWPOS_CENTERED,
	                                      SDL_WINDOWPOS_CENTERED,
	                                      g.width,
	                                      g.height,
	                                      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_GLContext glcontext = SDL_GL_CreateContext(window);
	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
	gl3wInit();

	// Setup ImGui binding
	ImGui_ImplSdlGL3_Init(window);

	ImGuiIO &io                          = ImGui::GetIO();
	io.IniFilename                       = NULL;
	std::string fontpath                 = get_asset_path(app.obvconfig.ParseStr("fontPath", "DroidSans.ttf"));
	if (g.font_size == 0.0f) g.font_size = app.obvconfig.ParseDouble("fontSize", 20.0f);
	io.Fonts->AddFontFromFileTTF(fontpath.c_str(), g.font_size);
	//	io.Fonts->AddFontDefault();

	// Main loop
	bool done             = false;
	bool preload_required = false;

	// Now that the configuration file is loaded in to BoardView, parse its settings.
	app.ConfigParse();

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
			ImGui_ImplSdlGL3_ProcessEvent(&event);

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
			snprintf(s, sizeof(s), "%s/.config/openboardview/obv.conf", homepath);
			app.obvconfig.Load(s);
			app.ConfigParse();
			clear_color = ImColor(app.m_colors.backgroundColor);
		}

		if (!(sleepout--)) {
			usleep(50000);
			sleepout = 0;
			continue;
		} // puts OBV to sleep if nothing is happening.

		ImGui_ImplSdlGL3_NewFrame(window);

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
			snprintf(scratch, sizeof(scratch), "OpenFlex Board Viewer - %s", app.fhistory.history[0]);
			SDL_SetWindowTitle(window, scratch);
			app.history_file_has_changed = 0;
		}

		// Rendering
		glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
		SDL_GL_SwapWindow(window);
	}

	// Cleanup
	ImGui_ImplSdlGL3_Shutdown();
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
