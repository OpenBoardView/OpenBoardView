#ifndef _PDFBRIDGESKIM_H_
#define _PDFBRIDGESKIM_H_

#ifdef __APPLE__

#include "PDFBridge.h"
#include "PDFFile.h"

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>

class PDFBridgeSkim : public PDFBridge {
private:
	std::string currentPdfPath;
	std::vector<std::string> openedPdfPaths; // PDFs opened by OBV — closed on exit

	// Background polling thread
	std::thread pollThread;
	std::atomic<bool> pollRunning{false};
	std::mutex selectionMutex;
	std::string pendingSelection;      // written by poll thread, read by main thread
	std::atomic<bool> selectionReady{false};

	std::string selection;             // last selection returned to caller
	std::string lastSearchTerm;
	int lastFoundPage{0};

	static std::string escapeForAppleScript(const std::string &s);
	static void runScript(const std::string &script);
	static std::string runScriptResult(const std::string &script);

	void pollLoop(); // background thread body

public:
	PDFBridgeSkim();
	~PDFBridgeSkim();

	void OpenDocument(const PDFFile &pdfFile) override;
	void CloseDocument() override;
	void DocumentSearch(const std::string &str, bool wholeWordsOnly, bool caseSensitive) override;
	bool HasNewSelection() override;
	std::string GetSelection() const override;
};

#endif // __APPLE__

#endif // _PDFBRIDGESKIM_H_
