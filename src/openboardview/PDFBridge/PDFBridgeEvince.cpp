#include "PDFBridgeEvince.h"

#include <SDL.h>

PDFBridgeEvince::PDFBridgeEvince() {
	GError *error = nullptr;

	dbusConnection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

	if (!dbusConnection) {
		if (error) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince: could not connect to DBUS, %s", error->message);
			g_error_free(error);
			error = nullptr;
		} else {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince: could not connect to DBUS");
		}
	}
}

PDFBridgeEvince::~PDFBridgeEvince() {
	g_object_unref(dbusConnection);
	dbusConnection = nullptr;
}

void PDFBridgeEvince::OpenDocument(const filesystem::path &pdfPath) {
	GError *error = nullptr;

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

	std::string uri = "file://" + filesystem::canonical(pdfPath).string();

	GVariant *owner = g_dbus_proxy_call_sync(daemonProxy, "FindDocument", g_variant_new("(sb)", uri.c_str(), true), G_DBUS_CALL_FLAGS_NONE, 15/*timeout*/, NULL, &error);
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

	const gchar *ownerStr;
	//= g_variant_get_string(owner, NULL);
	//const gchar *ownerStr = g_variant_get_type_string(owner);
	g_variant_get(owner, "(s)", &ownerStr);

	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeEvince FindDocuemnt owner: %s", ownerStr);

	windowProxy = g_dbus_proxy_new_sync(dbusConnection, G_DBUS_PROXY_FLAGS_NONE, NULL, ownerStr, "/org/gnome/evince/Window/0", "org.gnome.evince.Window", NULL, &error);
	if (!windowProxy) {
		if (error) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince windowProxy error: %s", error->message);
			g_error_free(error);
			error = nullptr;
		} else {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince windowProxy error");
		}
		return;
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

void PDFBridgeEvince::DocumentSearch(const std::string &str) {
	GError *error = nullptr;

	if (dbusConnection == nullptr) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince: DBUS not connected");
		return;
	}

	if (windowProxy == nullptr) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince attempting Search without any document open");
		return;
	}

	g_dbus_proxy_call_sync(windowProxy, "Search", g_variant_new("(s)", str.c_str()), G_DBUS_CALL_FLAGS_NONE, 5/*timeout*/, NULL, &error);

	if (error) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeEvince Search error: %s", error->message);
		g_error_free(error);
		error = nullptr;
	}
}
