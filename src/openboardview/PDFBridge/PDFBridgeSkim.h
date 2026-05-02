#ifndef _PDFBRIDGESKIM_H_
#define _PDFBRIDGESKIM_H_

#ifdef __APPLE__

#include "PDFBridge.h"
#include "PDFFile.h"

#include <string>
#include <atomic>
#include <chrono>

class PDFBridgeSkim : public PDFBridge {
private:
	std::string currentPdfPath;
	std::string selection;
	std::string lastPolledSelection;
	std::string lastSearchTerm;
	int lastFoundPage{0};
	std::atomic<bool> hasNewSelection{false};
	std::chrono::steady_clock::time_point lastPollTime{};

	static std::string escapeForAppleScript(const std::string &s);
	static void runScript(const std::string &script);
	static std::string runScriptResult(const std::string &script);

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
