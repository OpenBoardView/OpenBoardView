#ifndef _PDFBRIDGESUMATRA_H_
#define _PDFBRIDGESUMATRA_H_

#ifdef _WIN32

#include "platform.h" // Should be kept first

#include "PDFBridge.h"
#include "PDFFile.h"

#include <string>

#include "confparse.h"

class PDFBridgeSumatra : public PDFBridge {
private:
	static PDFBridgeSumatra *instance; // Keep a pointer to singleton instance for non-C++-aware DDE callback
	Confparse &obvConfig;

	std::wstring pdfPathString{};
	std::string reverseSearchStr{};
	bool reverseSearchStrChanged = false;

	const std::wstring szService{L"SUMATRA"}; // PDF software DDE server name
	const std::wstring szTopic{L"control"}; // PDF software DDE topic

	const std::wstring szServerService{L"OpenBoardView"}; // OBV DDE server name

	HSZ hszServerService = nullptr;
	HSZ hszServerTopic = nullptr;

	DWORD idInst = 0L;
	HCONV hConv = nullptr;
	HDDEDATA hDataNameService = nullptr; // OBV server name registered

	PDFBridgeSumatra(Confparse &obvconfig);

	static HDDEDATA CALLBACK DdeCallback(UINT uType, UINT uFmt, HCONV hconv, HSZ hsz1, HSZ hsz2, HDDEDATA hdata, ULONG_PTR dwData1, ULONG_PTR dwData2);
	bool InitializeDDE();
	bool StartDDEServer(const std::wstring &service, const std::wstring &topic);
	bool ConnectDDEClient(const std::wstring &service, const std::wstring &topic);
	bool ExecuteDDECommand(std::wstring ddeCmd);
	std::string GetDDEString(HSZ hsz);

public:
	~PDFBridgeSumatra();

	static PDFBridgeSumatra &GetInstance(Confparse &obvConfig);

	void OpenDocument(const PDFFile &pdfFile);
	void CloseDocument();
	void DocumentSearch(const std::string &str, bool wholeWordsOnly, bool caseSensitive);
	bool HasNewSelection();
	std::string GetSelection() const;

	bool ReverseSearch(const std::string &pdfPath, const std::string &searchStr);
};

#endif//_WIN32

#endif//_PDFBRIDGESUMATRA_H_
