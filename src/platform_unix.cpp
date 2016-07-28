#include "platform.h"
#include <gtk/gtk.h>
#include <string>

std::string show_file_picker() {
	std::string filename("");
	GtkWidget *parent, *dialog;

	if (!gtk_init_check(NULL, NULL))
		return filename;

	parent = gtk_window_new(GTK_WINDOW_TOPLEVEL);

 	dialog = gtk_file_chooser_dialog_new( "Open File",
					GTK_WINDOW(parent),
					GTK_FILE_CHOOSER_ACTION_OPEN,
					"_Cancel", GTK_RESPONSE_CANCEL,
					"_Open", GTK_RESPONSE_ACCEPT,
					NULL );

	/* Filter file types */
	GtkFileFilter *filter1 = gtk_file_filter_new();
	gtk_file_filter_set_name(filter1, "All Files");
	gtk_file_filter_add_pattern(filter1, "*");

	GtkFileFilter *filter2 = gtk_file_filter_new();
	gtk_file_filter_set_name(filter2, "BRD Files");
	gtk_file_filter_add_pattern(filter2, "*.brd");

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter1);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter2);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
 		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

	while (gtk_events_pending())
		gtk_main_iteration();

	gtk_widget_destroy(dialog);

 	while (gtk_events_pending())
 		gtk_main_iteration();

	return filename;
}
