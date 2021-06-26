#ifndef _PDFBRIDGEEVINCE_H_
#define _PDFBRIDGEEVINCE_H_

#include "PDFBridge.h"

#include <gio/gio.h>

class PDFBridgeEvince : public PDFBridge {
private:
	GDBusConnection* dbusConnection = nullptr;
	GDBusProxy *windowProxy = nullptr;
	GDBusProxy *daemonProxy = nullptr;
public:
	PDFBridgeEvince();
	~PDFBridgeEvince();

	void OpenDocument(const filesystem::path &pdfPath);
	void CloseDocument();
	void DocumentSearch(const std::string &str);
};

#endif//_PDFBRIDGEEVINCE_H_
