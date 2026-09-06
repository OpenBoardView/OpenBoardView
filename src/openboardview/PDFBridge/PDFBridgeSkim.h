#ifndef _PDFBRIDGESKIM_H_
#define _PDFBRIDGESKIM_H_

#ifdef ENABLE_PDFBRIDGE_SKIM

#include "PDFBridge.h"
#include "PDFFile.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include "confparse.h"

// macOS PDF bridge driving Skim (https://skim-app.sourceforge.io/) over AppleScript.
// Mirrors the PDFBridgeEvince lifecycle: lazy use in OpenDocument, path canonicalisation,
// existence check before load, and SDL_LogError on every failure path.
//
// Forward search (OBV -> Skim) runs synchronously on the caller/render thread, which is
// fine because it only fires on a button press. Reverse search (Skim -> OBV) reads Skim's
// text selection on a background thread at a configurable interval and stores it under a
// mutex; HasNewSelection()/GetSelection() -- called every frame from the render loop --
// only touch the cached value and never block on AppleScript.
class PDFBridgeSkim : public PDFBridge {
private:
	std::string appName; // Skim application name, configurable via obvConfig ("pdfSkimApplication")
	std::string pdfPath; // canonical path of the currently-open PDF, empty if none

	// Reverse-search polling. pollIntervalMs <= 0 disables reverse search entirely.
	int pollIntervalMs = 0;
	std::thread pollThread;
	std::atomic<bool> pollRunning{false};
	mutable std::mutex selectionMutex;
	std::string currentSelection;  // guarded by selectionMutex
	bool selectionChanged = false; // guarded by selectionMutex

	void startPolling();
	void stopPolling();
	void pollLoop();

public:
	PDFBridgeSkim(Confparse &obvConfig);
	~PDFBridgeSkim();

	void OpenDocument(const PDFFile &pdfFile);
	void CloseDocument();
	void DocumentSearch(const std::string &str, bool wholeWordsOnly, bool caseSensitive);
	bool HasNewSelection();
	std::string GetSelection() const;
};

#endif // ENABLE_PDFBRIDGE_SKIM

#endif //_PDFBRIDGESKIM_H_
