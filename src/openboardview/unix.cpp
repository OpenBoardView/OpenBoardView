#ifndef _WIN32

#include "platform.h"
#include "imgui/imgui.h"
#include "version.h"
#include <SDL.h>
#include <cstdint>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef ENABLE_GTK
#include <gtk/gtk.h>
#endif

#ifdef ENABLE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif

#ifdef ENABLE_GTK
#define declareFunc(x) decltype(x) *x
#define loadFunc(x) x = reinterpret_cast<decltype(x)>(SDL_LoadFunction(lib, #x));
namespace GTK {
bool loaded = false;
void *lib;

declareFunc(g_free);
declareFunc(gtk_dialog_get_type);
declareFunc(gtk_dialog_run);
declareFunc(gtk_events_pending);
declareFunc(gtk_file_chooser_add_filter);
declareFunc(gtk_file_chooser_dialog_new);
declareFunc(gtk_file_chooser_get_filename);
declareFunc(gtk_file_chooser_get_type);
declareFunc(gtk_file_filter_add_pattern);
declareFunc(gtk_file_filter_new);
declareFunc(gtk_file_filter_set_name);
declareFunc(gtk_init_check);
declareFunc(gtk_main_iteration);
declareFunc(gtk_widget_destroy);
declareFunc(gtk_window_get_type);
declareFunc(gtk_window_new);
declareFunc(g_type_check_instance_cast);

bool load() {
	if (loaded) return true;
	lib           = SDL_LoadObject("libgtk-3.so.0");
	if (!lib) lib = SDL_LoadObject("libgtk-x11-2.0.so.0");
	if (!lib) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot show open file dialog: no GTK library available.");
		return false;
	}

	loadFunc(g_free);
	loadFunc(gtk_dialog_get_type);
	loadFunc(gtk_dialog_run);
	loadFunc(gtk_events_pending);
	loadFunc(gtk_file_chooser_add_filter);
	loadFunc(gtk_file_chooser_dialog_new);
	loadFunc(gtk_file_chooser_get_filename);
	loadFunc(gtk_file_chooser_get_type);
	loadFunc(gtk_file_filter_add_pattern);
	loadFunc(gtk_file_filter_new);
	loadFunc(gtk_file_filter_set_name);
	loadFunc(gtk_init_check);
	loadFunc(gtk_main_iteration);
	loadFunc(gtk_widget_destroy);
	loadFunc(gtk_window_get_type);
	loadFunc(gtk_window_new);
	loadFunc(g_type_check_instance_cast);

	loaded = true;
	return true;
}
}
#undef declareFunc
#undef loadFunc
#define g_free GTK::g_free
#define gtk_dialog_get_type GTK::gtk_dialog_get_type
#define gtk_dialog_run GTK::gtk_dialog_run
#define gtk_events_pending GTK::gtk_events_pending
#define gtk_file_chooser_add_filter GTK::gtk_file_chooser_add_filter
#define gtk_file_chooser_dialog_new GTK::gtk_file_chooser_dialog_new
#define gtk_file_chooser_get_filename GTK::gtk_file_chooser_get_filename
#define gtk_file_chooser_get_type GTK::gtk_file_chooser_get_type
#define gtk_file_filter_add_pattern GTK::gtk_file_filter_add_pattern
#define gtk_file_filter_new GTK::gtk_file_filter_new
#define gtk_file_filter_set_name GTK::gtk_file_filter_set_name
#define gtk_init_check GTK::gtk_init_check
#define gtk_main_iteration GTK::gtk_main_iteration
#define gtk_widget_destroy GTK::gtk_widget_destroy
#define gtk_window_get_type GTK::gtk_window_get_type
#define gtk_window_new GTK::gtk_window_new
#define g_type_check_instance_cast GTK::g_type_check_instance_cast

const filesystem::path show_file_picker(bool filterBoards) {
	std::string path;
	if (!GTK::load()) return path;

	GtkWidget *parent, *dialog;
	GtkFileFilter *filter            = gtk_file_filter_new();
	GtkFileFilter *filter_everything = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Boards");
	gtk_file_filter_add_pattern(filter, "*.[aA][sS][cC]");
	gtk_file_filter_add_pattern(filter, "*.[bB][oO][mM]");
	gtk_file_filter_add_pattern(filter, "*.[bB][rR][dD]");
	gtk_file_filter_add_pattern(filter, "*.[bB][dD][vV]");
	gtk_file_filter_add_pattern(filter, "*.[bB][vV][rR]");
	gtk_file_filter_add_pattern(filter, "*.[cC][aA][dD]");
	gtk_file_filter_add_pattern(filter, "*.[cC][sS][tT]");
	gtk_file_filter_add_pattern(filter, "*.[pP][cC][bB][dD][oO][cC]");
	gtk_file_filter_add_pattern(filter, "*.[fF][zZ]");
	gtk_file_filter_add_pattern(filter, "*.[pP][cC][bB]");

	gtk_file_filter_set_name(filter_everything, "All");
	gtk_file_filter_add_pattern(filter_everything, "*");

	if (!gtk_init_check(NULL, NULL)) return {};

	parent = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	dialog = gtk_file_chooser_dialog_new("Open File",
	                                     GTK_WINDOW(parent),
	                                     GTK_FILE_CHOOSER_ACTION_OPEN,
	                                     "_Cancel",
	                                     GTK_RESPONSE_CANCEL,
	                                     "_Open",
	                                     GTK_RESPONSE_ACCEPT,
	                                     NULL);

	if (filterBoards) {
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_everything);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		if (filename) {
			path = std::string(filename);
			g_free(filename);
		}
	}

	while (gtk_events_pending()) gtk_main_iteration();

	gtk_widget_destroy(dialog);

	while (gtk_events_pending()) gtk_main_iteration();

	return path;
}
#elif !defined(__APPLE__)
const filesystem::path show_file_picker(bool filterBoards) { // dummy function when not building for OS X and GTK not available
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot show open file dialog: not built in.");
	return std::string();
}
#endif

#ifdef ENABLE_FONTCONFIG
// Thanks to http://stackoverflow.com/a/14634033/1447751
const std::string get_font_path(const std::string &name) {
	std::string path;
	const FcChar8 *fcname = reinterpret_cast<const FcChar8 *>(name.c_str());
	FcConfig *config      = FcInitLoadConfigAndFonts();

	// configure the search pattern,
	// assume "name" is a std::string with the desired font name in it
	FcPattern *pat = FcNameParse(fcname);
	FcConfigSubstitute(config, pat, FcMatchPattern);
	FcDefaultSubstitute(pat);

	FcResult result;
	// find the font
	FcPattern *font = FcFontMatch(config, pat, &result);
	if (font) {
		FcChar8 *fcpath   = NULL;
		FcChar8 *fcfamily = NULL;
		if (FcPatternGetString(font, FC_FAMILY, 0, &fcfamily) == FcResultMatch &&
		    (name.empty() || !FcStrCmpIgnoreCase(fcname, fcfamily)) // Empty name means searching for default font, otherwise make
		                                                            // sure the returned font is exactly what we searched for
		    &&
		    FcPatternGetString(font, FC_FILE, 0, &fcpath) == FcResultMatch)
			path = std::string(reinterpret_cast<char *>(fcpath));
		FcPatternDestroy(font);
	}

	FcPatternDestroy(pat);
	return path;
}
#endif

#ifndef __APPLE__
// Return an environment variable value in an std::string
const std::string get_env_var(const std::string varname) {
	std::string envVar;
	const char *cEnvVar = std::getenv(varname.c_str());
	if (cEnvVar) envVar = std::string(cEnvVar); // cEnvVar may be null
	return envVar;
}

// Return true upon successful directory creation or if path was an existing directory
bool create_dir(const std::string &path) {
	struct stat st;
	int sr;
	sr = stat(path.c_str(), &st);
	if (sr == -1) {
		mkdir(path.c_str(), S_IRWXU);
		sr = stat(path.c_str(), &st);
	}
	if ((sr == 0) && (S_ISDIR(st.st_mode))) return true;
	return false;
}

// Recursively create directories
bool create_dirs(const std::string &path) {
	for (size_t pos = 0; (pos = path.find("/", pos + 1)) != std::string::npos;) {
		if (!create_dir(path.substr(0, pos + 1))) return false;
	}
	return true;
}

const std::string get_user_dir(const UserDir userdir) {
	std::string path;
	std::string envVar;

	if (userdir == UserDir::Config)
		envVar = get_env_var("XDG_CONFIG_HOME");
	else if (userdir == UserDir::Data)
		envVar = get_env_var("XDG_DATA_HOME");

	if (envVar.empty()) {
		envVar = get_env_var("HOME");
		if (!envVar.empty()) {
			path += std::string(envVar);
			if (userdir == UserDir::Config)
				path += "/.config";
			else if (userdir == UserDir::Data)
				path += "/.local/share";
		}
	} else {
		path += std::string(envVar);
	}
	if (!path.empty()) {
		path += "/" OBV_NAME "/";
		if (create_dirs(path)) return path; // Check if dir already exists and create it otherwise
	}
	return "./"; // Something went wrong, use current dir
}
#endif

#endif
