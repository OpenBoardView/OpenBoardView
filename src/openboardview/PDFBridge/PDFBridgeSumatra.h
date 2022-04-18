#ifndef _PDFBRIDGESUMATRA_H_
#define _PDFBRIDGESUMATRA_H_

#ifdef _WIN32

#include "platform.h" // Should be kept first

#include "PDFBridge.h"

#include <string>

class PDFBridgeSumatra : public PDFBridge {
private:
	std::wstring pdfPathString{};

	const wchar_t *szService = L"SUMATRA";
	const wchar_t *szTopic = L"control";

	DWORD idInst = 0L;
	HCONV hConv = nullptr;

	static HDDEDATA CALLBACK DdeCallback(UINT uType, UINT uFmt, HCONV hconv, HSZ hsz1, HSZ hsz2, HDDEDATA hdata, DWORD dwData1, DWORD dwData2);
public:
	PDFBridgeSumatra();
	~PDFBridgeSumatra();

	void OpenDocument(const filesystem::path &pdfPath);
	void CloseDocument();
	void DocumentSearch(const std::string &str, bool wholeWordsOnly, bool caseSensitive);
	bool HasNewSelection();
	std::string GetSelection() const;
};

#endif//_WIN32

#endif//_PDFBRIDGESUMATRA_H_
