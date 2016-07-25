#ifndef _WIN32

#define _CRT_SECURE_NO_WARNINGS 1
#include "platform.h"
#include "imgui.h"
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <assert.h>

#ifndef __APPLE__
#include <gtk/gtk.h>
#endif  // ! __APPLE__

char *file_as_buffer(size_t *buffer_size, const char *utf8_filename) {
	std::ifstream file;
	file.open(utf8_filename,  std::ios::in | std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		std::cerr << "Error opening " << utf8_filename << ": " << strerror(errno) << std::endl;
		*buffer_size = 0;
		return nullptr;
	}

	std::streampos sz = file.tellg();
	*buffer_size = sz;
	char *buf = (char *)malloc(sz);
	file.seekg(0, std::ios::beg);
	file.read(buf, sz);
	assert(file.gcount() == sz);
	file.close();

	return buf;
}

#ifndef __APPLE__
/* These are defined in platform_osx.mm */

char *show_file_picker() {
	char *path = nullptr;
	GtkWidget *parent, *dialog;

	if (!gtk_init_check(NULL, NULL))
		return nullptr;

	parent = gtk_window_new(GTK_WINDOW_TOPLEVEL);

 	dialog = gtk_file_chooser_dialog_new( "Open File",
					GTK_WINDOW(parent),
					GTK_FILE_CHOOSER_ACTION_OPEN,
					"_Cancel", GTK_RESPONSE_CANCEL,
					"_Open", GTK_RESPONSE_ACCEPT,
					NULL );

	/* Filter file types */
	// TODO: use a static/const array of filters
	GtkFileFilter *filter1 = gtk_file_filter_new();
	gtk_file_filter_set_name(filter1, "All Supported Files");
	gtk_file_filter_add_pattern(filter1, "*.brd");
	gtk_file_filter_add_pattern(filter1, "*.bdv");

	GtkFileFilter *filter2 = gtk_file_filter_new();
	gtk_file_filter_set_name(filter2, "BRD Files");
	gtk_file_filter_add_pattern(filter2, "*.brd");

	GtkFileFilter *filter3 = gtk_file_filter_new();
	gtk_file_filter_set_name(filter3, "BDV Files");
	gtk_file_filter_add_pattern(filter3, "*.bdv");

	GtkFileFilter *filter4 = gtk_file_filter_new();
	gtk_file_filter_set_name(filter4, "All Files");
	gtk_file_filter_add_pattern(filter4, "*");

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter1);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter2);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter3);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter4);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename = nullptr;

 		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		size_t len = strlen(filename);
		path = (char *)malloc(len+1);
		memcpy(path, filename, len+1);
		if (!path ) {
			g_free(filename);
			gtk_widget_destroy(dialog);
			return nullptr;
		}
		g_free(filename);
	}

	while (gtk_events_pending())
		gtk_main_iteration();

	gtk_widget_destroy(dialog);

 	while (gtk_events_pending())
 		gtk_main_iteration();

	return path;
}

#endif // ! __APPLE__

#endif // ! _WIN32
