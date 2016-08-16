#ifndef _WIN32

#define _CRT_SECURE_NO_WARNINGS 1
#include "platform.h"
#include "imgui/imgui.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <stdint.h>

#ifdef ENABLE_GTK
#include <gtk/gtk.h>
#endif

#ifdef ENABLE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif

char *file_as_buffer(size_t *buffer_size, const char *utf8_filename) {
	std::ifstream file;
	file.open(utf8_filename, std::ios::in | std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		std::cerr << "Error opening " << utf8_filename << ": " << strerror(errno) << std::endl;
		*buffer_size = 0;
		return nullptr;
	}

	std::streampos sz = file.tellg();
	*buffer_size      = sz;
	char *buf         = (char *)malloc(sz);
	file.seekg(0, std::ios::beg);
	file.read(buf, sz);
	assert(file.gcount() == sz);
	file.close();

	return buf;
}

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

char *show_file_picker() {
	char *path = nullptr;
	if (!GTK::load()) return path;

	GtkWidget *parent, *dialog;
	GtkFileFilter *filter            = gtk_file_filter_new();
	GtkFileFilter *filter_everything = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Boards");
	gtk_file_filter_add_pattern(filter, "*.[bB][rR][dD]");
	gtk_file_filter_add_pattern(filter, "*.[bB][dD][vV]");
	gtk_file_filter_add_pattern(filter, "*.[bB][vV][rR]");
	gtk_file_filter_add_pattern(filter, "*.[fF][zZ]");

	gtk_file_filter_set_name(filter_everything, "All");
	gtk_file_filter_add_pattern(filter_everything, "*");

	if (!gtk_init_check(NULL, NULL)) return nullptr;

	parent = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	dialog = gtk_file_chooser_dialog_new("Open File",
	                                     GTK_WINDOW(parent),
	                                     GTK_FILE_CHOOSER_ACTION_OPEN,
	                                     "_Cancel",
	                                     GTK_RESPONSE_CANCEL,
	                                     "_Open",
	                                     GTK_RESPONSE_ACCEPT,
	                                     NULL);

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_everything);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;

		filename   = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		size_t len = strlen(filename);
		path       = (char *)malloc(len + 1);
		memcpy(path, filename, len + 1);
		if (!path) {
			g_free(filename);
			gtk_widget_destroy(dialog);
			return nullptr;
		}
		g_free(filename);
	}

	while (gtk_events_pending()) gtk_main_iteration();

	gtk_widget_destroy(dialog);

	while (gtk_events_pending()) gtk_main_iteration();

	return path;
}
#elif !defined(__APPLE__)
char *show_file_picker() { // dummy function when not building for OS X and GTK not available
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot show open file dialog: not built in.");
	return nullptr;
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

#endif
