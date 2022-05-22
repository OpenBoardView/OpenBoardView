#ifndef _PDFBRIDGEEVINCE_H_
#define _PDFBRIDGEEVINCE_H_

#ifdef ENABLE_PDFBRIDGE_EVINCE

#include "PDFBridge.h"
#include "PDFFile.h"

#include <string>

#include <gio/gio.h>


class PDFBridgeEvince : public PDFBridge {
private:
	GDBusConnection* dbusConnection = nullptr;
	GDBusProxy *windowProxy = nullptr;
	GDBusProxy *daemonProxy = nullptr;

	std::string selection = "";

	static void OnSignal(GDBusProxy *proxy, gchar *senderName, gchar *signalName, GVariant *parameters, gpointer userData);
public:
	PDFBridgeEvince();
	~PDFBridgeEvince();

	void OpenDocument(const PDFFile &pdfFile);
	void CloseDocument();
	void DocumentSearch(const std::string &str, bool wholeWordsOnly, bool caseSensitive);
	bool HasNewSelection();
	std::string GetSelection() const;
};

#endif

#endif//_PDFBRIDGEEVINCE_H_
