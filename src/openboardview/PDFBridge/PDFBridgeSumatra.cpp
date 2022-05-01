#include "PDFBridgeSumatra.h"

#include <SDL.h>

#include "win32.h"
#include "utils.h"

PDFBridgeSumatra::PDFBridgeSumatra() {
}

PDFBridgeSumatra::~PDFBridgeSumatra() {
	if (this->hszServerService != nullptr) {
		DdeFreeStringHandle(this->idInst, hszServerService);
	}
	if (this->hszServerTopic != nullptr) {
		DdeFreeStringHandle(this->idInst, hszServerTopic);
	}
	if (this->hConv != nullptr) {
		DdeDisconnect(this->hConv);
		hConv = nullptr;
	}
	if (this->idInst != 0L) {
		DdeUninitialize(this->idInst);
		idInst = 0L;
	}
}

PDFBridgeSumatra &PDFBridgeSumatra::GetInstance() {
	static PDFBridgeSumatra instance{};
	return instance;
}

std::string PDFBridgeSumatra::GetDDEString(HSZ hsz) {
	size_t len = DdeQueryStringW(this->idInst, hsz, nullptr, 0, CP_WINUNICODE); // Lengh of data in characters (trailing NULL char not included)
	std::wstring wstr(len + 1, '\0');
	DdeQueryStringW(this->idInst, hsz, wstr.data(), wstr.capacity(), CP_WINUNICODE); // Lengh of data in characters (trailing NULL char not included)
	return utf16_to_utf8(wstr);
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
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeSumatra DdeCallback, uType=%d", uType);

	PDFBridgeSumatra &pdfBridgeSumatra = PDFBridgeSumatra::GetInstance();

	switch (uType) {
		case XTYP_CONNECT: {
			if (DdeCmpStringHandles(hsz2, pdfBridgeSumatra.hszServerService)) {
				SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeSumatra DdeCallback Client request connection to unhandled service '%s'", pdfBridgeSumatra.GetDDEString(hsz2).c_str());
				return FALSE;
			} else if (DdeCmpStringHandles(hsz1, pdfBridgeSumatra.hszServerTopic)) {
				SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeSumatra DdeCallback Client request connection to unhandled topic '%s'", pdfBridgeSumatra.GetDDEString(hsz1).c_str());
				return FALSE;
			} else {
				SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeSumatra DdeCallback Client request connection to service '%s', topic '%s'", pdfBridgeSumatra.GetDDEString(hsz2).c_str(), pdfBridgeSumatra.GetDDEString(hsz1).c_str());
				return reinterpret_cast<HDDEDATA>(TRUE);
			}

			break;
		}

		case XTYP_CONNECT_CONFIRM: {
			SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "%s", "PDFBridgeSumatra DdeCallback Client connected");
			break;
		}

		case XTYP_EXECUTE: {
			size_t len = DdeGetData(hdata, nullptr, 0, 0); // Length of data in bytes (trailing NULL char included)
			std::wstring wstr(len / 2, '\0');
			DdeGetData(hdata, reinterpret_cast<LPBYTE>(wstr.data()), wstr.capacity() * 2, 0);
			std::string str = utf16_to_utf8(wstr);

			SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeSumatra DdeCallback Client execute: %s", str.c_str());

			if (!str.compare(0, 9, "[Search(\"") && !str.compare(str.size() - 3, 3, "\")]")) {
				auto searchStr = str.substr(9, str.size() - 9 - 3); // Split parameters between "[Search(" and ")]"

				SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeSumatra DdeCallback Client Search '%s'", searchStr.c_str());
				pdfBridgeSumatra.reverseSearchStr = searchStr;
				pdfBridgeSumatra.reverseSearchStrChanged = true;

				return reinterpret_cast<HDDEDATA>(DDE_FACK);
			} else {
				SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra DdeCallback Client execute unknown request: %s", str.c_str());
				return reinterpret_cast<HDDEDATA>(DDE_FNOTPROCESSED);
			}

			break;
		}

		default: {
			SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeSumatra DdeCallback Unknown request: 0x%04x", uType);
		}
	}

	return nullptr;
}

bool PDFBridgeSumatra::HasNewSelection() {
	if (reverseSearchStrChanged) {
		reverseSearchStrChanged = false;
		return true;
	} else {
		return false;
	}
}

std::string PDFBridgeSumatra::GetSelection() const {
	return reverseSearchStr;
}

bool PDFBridgeSumatra::InitializeDDE() {
	if (this->idInst != 0L) { // Already initialized
		return true;
	}

	UINT ret = DdeInitializeW(&this->idInst, DdeCallback, APPCLASS_STANDARD, 0);
	if (ret != DMLERR_NO_ERROR) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra DDE initialization failed: 0x%04x", ret);
		return false;
	}

	return true;
}

bool PDFBridgeSumatra::StartDDEServer(const std::wstring &service, const std::wstring &topic) {
	if (this->hDataNameService != nullptr) { // Already started
		return true;
	}

	this->hszServerService = DdeCreateStringHandleW(this->idInst, service.c_str(), 0);
	this->hszServerTopic = DdeCreateStringHandleW(this->idInst, topic.c_str(), 0);

	HDDEDATA hDataNameService = DdeNameService(this->idInst, hszServerService, nullptr, DNS_REGISTER);

	if (hDataNameService == nullptr) {
		UINT ret = DdeGetLastError(this->idInst);
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra DdeNameService failed: 0x%04x", ret);
		return false;
	}

	return true;
}


bool PDFBridgeSumatra::ConnectDDEClient(const std::wstring &service, const std::wstring &topic) {
	if (this->hConv != nullptr) { // Already connected
		return true;
	}

	HSZ hszService = DdeCreateStringHandleW(this->idInst, service.c_str(), 0);
	HSZ hszTopic = DdeCreateStringHandleW(this->idInst, topic.c_str(), 0);

	this->hConv = DdeConnect(idInst, hszService, hszTopic, nullptr);

	DdeFreeStringHandle(this->idInst, hszService);
	DdeFreeStringHandle(this->idInst, hszTopic);

    if (this->hConv == nullptr) {
		UINT ret = DdeGetLastError(this->idInst);
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra DDE connection to service '%s' with topic '%s' failed: 0x%04x", utf16_to_utf8(service).c_str(), utf16_to_utf8(topic).c_str(), ret);
        return false;
    }

	return true;
}

bool PDFBridgeSumatra::ExecuteDDECommand(std::wstring ddeCmd) {
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeSumatra ExecuteDDECommand ddeCmd: %s", utf16_to_utf8(ddeCmd).c_str());

	HDDEDATA hData = DdeCreateDataHandle(this->idInst, reinterpret_cast<LPBYTE>(ddeCmd.data()), (ddeCmd.size() + 1) * sizeof(wchar_t), 0, nullptr, CF_TEXT, 0);

    if (hData == nullptr)   {
		UINT ret = DdeGetLastError(this->idInst);
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra ExecuteDDECommand DdeCreateDataHandle failed: 0x%04x", ret);
        return false;
    }

	hData = DdeClientTransaction(reinterpret_cast<LPBYTE>(hData), -1, this->hConv, nullptr, 0, XTYP_EXECUTE, 10000/*10 seconds timeout*/, nullptr);

    if (hData == nullptr)   {
		UINT ret = DdeGetLastError(this->idInst);
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra ExecuteDDECommand DdeClientTransaction failed: 0x%04x", ret);
        return false;
    }

	return true;
}

void PDFBridgeSumatra::OpenDocument(const filesystem::path &pdfPath) {
	if (!filesystem::exists(pdfPath)) { // PDF file does not exist, do not attempt to load
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra PDF file does not exist");
		return;
	}
	this->pdfPathString = filesystem::canonical(pdfPath).wstring();

	if (!this->InitializeDDE())
		return;

	this->StartDDEServer(this->szServerService, this->pdfPathString);

	//TODO: Spawn Sumatra PDF process
	if (!this->ConnectDDEClient(this->szService, this->szTopic))
		return;

	std::wstring ddeCmd = L"[Open(\"" + this->pdfPathString + L"\",0,1,1)]";

	ExecuteDDECommand(ddeCmd);
}

void PDFBridgeSumatra::CloseDocument() {
	pdfPathString.clear();
	//TODO: unregister DDE server, disconnect client
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

	this->ExecuteDDECommand(ddeCmd);
}

bool PDFBridgeSumatra::ReverseSearch(const std::string &pdfPath, const std::string &searchStr) {
	std::string ddeCmd = "[Search(\"" + searchStr + "\")]";
	auto wDdeCmd = utf8_to_utf16(ddeCmd);

	if (!this->InitializeDDE())
		return false;

	if (!this->ConnectDDEClient(this->szServerService, utf8_to_utf16(pdfPath)))
		return false;

	return this->ExecuteDDECommand(wDdeCmd);
}
