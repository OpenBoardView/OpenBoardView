#include "PDFBridgeEvince.h"

#include <SDL.h>

PDFBridgeEvince::PDFBridgeEvince() {
}

PDFBridgeEvince::~PDFBridgeEvince() {
	if (dbusConnection != nullptr)
		g_object_unref(dbusConnection);
	dbusConnection = nullptr;
}

void PDFBridgeEvince::OnSignal(GDBusProxy *proxy, gchar *senderName, gchar *signalName, GVariant *parameters, gpointer userData) {
	auto &pdfBridge = *reinterpret_cast<PDFBridgeEvince*>(userData); // Unsafe, not much we can do about it since gdbus doesn't provide a proper C++11 API with functors for callback

	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeEvince Signal: '%s' from '%s'", signalName, senderName);

	if (std::string{signalName} == "SelectionChanged") {
		gchar *selectedText = nullptr;
		g_variant_get(parameters, "(s)", &selectedText);

		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeEvince SelectionChanged: '%s'", selectedText);

		pdfBridge.selection = selectedText;
	}
}

bool PDFBridgeEvince::HasNewSelection() {
	auto oldSelection = selection;

	// Process signals
	g_main_context_iteration(g_main_context_default(), false);

	if (oldSelection != selection) {
		return true;
	} else {
		return false;
	}
}

std::string PDFBridgeEvince::GetSelection() const {
	return selection;
}

void PDFBridgeEvince::OpenDocument(const PDFFile &pdfFile) {
	auto pdfPath = pdfFile.getPath();

	if (!filesystem::exists(pdfPath)) { // PDF file does not exist, do not attempt to load
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince PDF file does not exist");
		return;
	}

	GError *error = nullptr;

	if (!dbusConnection)
		dbusConnection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

	if (!dbusConnection) {
		if (error) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince: could not connect to DBUS, %s", error->message);
			g_error_free(error);
			error = nullptr;
		} else {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince: could not connect to DBUS");
		}
		return;
	}

	daemonProxy = g_dbus_proxy_new_sync(dbusConnection, G_DBUS_PROXY_FLAGS_NONE, NULL, "org.gnome.evince.Daemon", "/org/gnome/evince/Daemon", "org.gnome.evince.Daemon", NULL, &error);
	if (!daemonProxy) {
		if (error) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince daemonProxy error: %s", error->message);
			g_error_free(error);
			error = nullptr;
		} else {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince daemonProxy error");
		}
		return;
	}

	std::string canonical_path = filesystem::canonical(pdfPath).string();

	GFile *gfile = g_file_new_for_path(canonical_path.c_str());
	char *uri = g_file_get_uri(gfile);
	g_object_unref(gfile);

	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeEvince FindDocument: %s", uri);

	GVariant *owner = g_dbus_proxy_call_sync(daemonProxy, "FindDocument", g_variant_new("(sb)", uri, true), G_DBUS_CALL_FLAGS_NONE, 10000/*10 seconds timeout*/, NULL, &error);
	if (!owner) {
		if (error) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince FindDocument error: %s", error->message);
			g_error_free(error);
			error = nullptr;
		} else {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince FindDocument error");
		}
		return;
	}

	g_free(uri);
	uri = nullptr;

	const gchar *ownerStr;
	//= g_variant_get_string(owner, NULL);
	//const gchar *ownerStr = g_variant_get_type_string(owner);
	g_variant_get(owner, "(s)", &ownerStr);

	if (ownerStr[0] == '\0') {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince empty owner");
		g_variant_unref(owner);
		return;
	}

	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeEvince FindDocument owner: %s", ownerStr);

	windowProxy = g_dbus_proxy_new_sync(dbusConnection, G_DBUS_PROXY_FLAGS_NONE, NULL, ownerStr, "/org/gnome/evince/Window/0", "org.gnome.evince.Window", NULL, &error);
	if (!windowProxy) {
		if (error) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince windowProxy error: %s", error->message);
			g_error_free(error);
			error = nullptr;
		} else {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince windowProxy error");
		}
	} else {
		// Connect DBus signal for Evince Selection
		g_signal_connect(windowProxy, "g-signal", G_CALLBACK(&PDFBridgeEvince::OnSignal), this);
	}

	g_variant_unref(owner);
}

void PDFBridgeEvince::CloseDocument() {
	/* No way to close document in Evince for now so just clean state up */

	g_object_unref(daemonProxy);
	daemonProxy = nullptr;
	g_object_unref(windowProxy);
	windowProxy = nullptr;
}

void PDFBridgeEvince::DocumentSearch(const std::string &str, bool wholeWordsOnly, bool caseSensitive) {
	GError *error = nullptr;

	if (dbusConnection == nullptr) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince: DBUS not connected");
		return;
	}

	if (windowProxy == nullptr) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince attempting Search without any document open");
		return;
	}

	g_dbus_proxy_call_sync(windowProxy, "Search", g_variant_new("(sbb)", str.c_str(), wholeWordsOnly, caseSensitive), G_DBUS_CALL_FLAGS_NONE, 5000/*5 seconds timeout*/, NULL, &error);

	if (error) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince Search error: %s", error->message);
		g_error_free(error);
		error = nullptr;
	}
}
