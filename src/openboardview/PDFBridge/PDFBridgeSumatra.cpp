#include "PDFBridgeSumatra.h"

#include <SDL.h>

#include "win32.h"

PDFBridgeSumatra::PDFBridgeSumatra() {
}

PDFBridgeSumatra::~PDFBridgeSumatra() {
	if (this->hConv != nullptr) {
		DdeDisconnect(this->hConv);
		hConv = nullptr;
	}
	if (this->idInst != 0L) {
		DdeUninitialize(this->idInst);
		idInst = 0L;
	}
}

HDDEDATA CALLBACK PDFBridgeSumatra::DdeCallback(
    UINT uType,     // Transaction type.
    UINT uFmt,      // Clipboard data format.
    HCONV hconv,    // Handle to the conversation.
    HSZ hsz1,       // Handle to a string.
    HSZ hsz2,       // Handle to a string.
    HDDEDATA hdata, // Handle to a global memory object.
    DWORD dwData1,  // Transaction-specific data.
    DWORD dwData2) { // Transaction-specific data.
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "%s", "PDFBridgeEvince DdeCallback");
	return 0;
}

bool PDFBridgeSumatra::HasNewSelection() {
	//TODO: NotImplemented
	return false;
}

std::string PDFBridgeSumatra::GetSelection() const {
	//TODO: NotImplemented
	return  {};
}

void PDFBridgeSumatra::OpenDocument(const filesystem::path &pdfPath) {
	if (!filesystem::exists(pdfPath)) { // PDF file does not exist, do not attempt to load
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra PDF file does not exist");
		return;
	}
	this->pdfPathString = filesystem::canonical(pdfPath).wstring();

	if (this->idInst == 0L) {
		UINT ret = DdeInitializeW(&this->idInst, DdeCallback, APPCLASS_STANDARD | APPCMD_CLIENTONLY, 0);
		if (ret != DMLERR_NO_ERROR) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra DDE initialization failed: 0x%04x", ret);
			return;
		}
	}

	if (this->hConv == nullptr) {
		HSZ hszService = DdeCreateStringHandleW(this->idInst, szService, 0);
		HSZ hszTopic = DdeCreateStringHandleW(this->idInst, szTopic, 0);

		this->hConv = DdeConnect(idInst, hszService, hszTopic, nullptr);

		DdeFreeStringHandle(this->idInst, hszService);
		DdeFreeStringHandle(this->idInst, hszTopic);
	}

    if (this->hConv == nullptr) {
		UINT ret = DdeGetLastError(this->idInst);
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra DDE connection failed: 0x%04x", ret);
        return;
    }

	std::wstring ddeCmd = L"[Open(\"" + this->pdfPathString + L"\",0,1,1)]";
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeEvince ddeCmd: %s", utf16_to_utf8(ddeCmd).c_str());

	HDDEDATA hData = DdeCreateDataHandle(this->idInst, reinterpret_cast<LPBYTE>(ddeCmd.data()), (ddeCmd.size() + 1) * sizeof(wchar_t), 0, nullptr, CF_TEXT, 0);

    if (hData == nullptr)   {
		UINT ret = DdeGetLastError(this->idInst);
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra OpenDocument DdeCreateDataHandle failed: 0x%04x", ret);
        return;
    }

	hData = DdeClientTransaction(reinterpret_cast<LPBYTE>(hData), -1, this->hConv, nullptr, 0, XTYP_EXECUTE, 10000/*10 seconds timeout*/, nullptr);

    if (hData == nullptr)   {
		UINT ret = DdeGetLastError(this->idInst);
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra OpenDocument DdeClientTransaction failed: 0x%04x", ret);
        return;
    }
}

void PDFBridgeSumatra::CloseDocument() {
	pdfPathString.clear();
}

void PDFBridgeSumatra::DocumentSearch(const std::string &str, bool wholeWordsOnly, bool caseSensitive) {
	if (this->idInst == 0L || this->hConv == nullptr) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", "PDFBridgeSumatra DocumentSearch DDE not initialized");
	}

	auto strCopy = str;
	// With SumatraPDF, addings a space before and after the search string effectively performs a "whole words only" search
	if (wholeWordsOnly) {
		strCopy = " " + strCopy + " ";
	}

	auto wstr = utf8_to_utf16(strCopy);
	std::wstring ddeCmd = L"[Search(\"" + this->pdfPathString + L"\",\"" + wstr + L"\"," + (caseSensitive ? L"1" : L"0") + L")]";
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeEvince ddeCmd: %s", utf16_to_utf8(ddeCmd).c_str());

	HDDEDATA hData = DdeCreateDataHandle(this->idInst, reinterpret_cast<LPBYTE>(ddeCmd.data()), (ddeCmd.size() + 1) * sizeof(wchar_t), 0, nullptr, CF_TEXT, 0);

    if (hData == nullptr)   {
		UINT ret = DdeGetLastError(this->idInst);
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra DocumentSearch DdeCreateDataHandle failed: 0x%04x", ret);
        return;
    }

	hData = DdeClientTransaction(reinterpret_cast<LPBYTE>(hData), -1, this->hConv, nullptr, 0, XTYP_EXECUTE, 10000/*10 seconds timeout*/, nullptr);

    if (hData == nullptr)   {
		UINT ret = DdeGetLastError(this->idInst);
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra DocumentSearch DdeClientTransaction failed: 0x%04x", ret);
        return;
    }
}
