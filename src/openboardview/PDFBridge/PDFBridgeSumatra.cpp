#include "PDFBridgeSumatra.h"

#include <cassert>
#include <SDL.h>

#include "win32.h"
#include "utils.h"

PDFBridgeSumatra *PDFBridgeSumatra::instance = nullptr;

PDFBridgeSumatra::PDFBridgeSumatra(Confparse &obvConfig) : obvConfig(obvConfig) {
}

PDFBridgeSumatra::~PDFBridgeSumatra() {
	this->CloseDocument();

	if (this->hszServerService != nullptr) {
		DdeFreeStringHandle(this->idInst, hszServerService);
	}
	if (this->hszServerTopic != nullptr) {
		DdeFreeStringHandle(this->idInst, hszServerTopic);
	}
	if (this->idInst != 0L) {
		DdeUninitialize(this->idInst);
		idInst = 0L;
	}
}

PDFBridgeSumatra &PDFBridgeSumatra::GetInstance(Confparse &obvConfig) {
	static PDFBridgeSumatra instance{obvConfig};
	PDFBridgeSumatra::instance = &instance;
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


	assert(PDFBridgeSumatra::instance != nullptr);
	PDFBridgeSumatra &pdfBridgeSumatra = *PDFBridgeSumatra::instance;

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

				// Push event to SDL queue in order to refresh GUI
				SDL_Event sdlEvent;
				sdlEvent.type = SDL_USEREVENT;
				SDL_PushEvent(&sdlEvent);

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

void PDFBridgeSumatra::OpenDocument(const PDFFile &pdfFile) {
	auto pdfPath = pdfFile.getPath();

	if (!filesystem::exists(pdfPath)) { // PDF file does not exist, do not attempt to load
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra PDF file '%s' does not exist", pdfPath.string().c_str());
		return;
	}
	this->pdfPathString = filesystem::canonical(pdfPath).wstring();

	if (!this->InitializeDDE())
		return;

	this->StartDDEServer(this->szServerService, this->pdfPathString);

	// Spawn Sumatra PDF process if DDE connection failed (processs probably not running)
	if (!this->ConnectDDEClient(this->szService, this->szTopic)) {
		// Get path of current executable to use SumatraPDF executable from the same directory
		std::wstring currentExePathStr(32767, '\0');
		DWORD ret = GetModuleFileNameW(nullptr, currentExePathStr.data(),  currentExePathStr.capacity());
		if (ret == 0L) {
			ret = GetLastError();
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra OpenDocument GetModuleFileNameW failed: 0x%08lx", ret);
			return;
		}
		filesystem::path currentExePath{currentExePathStr};

		filesystem::path pdfSoftwarePath{this->obvConfig.ParseStr("pdfSoftwarePath", "SumatraPDF.exe")};
		if (pdfSoftwarePath.empty()) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", "PDFBridgeSumatra OpenDocument: PDF software executable path empty");
			return;
		}

		std::error_code ec;
		std::wstring exec = filesystem::relative(pdfSoftwarePath, currentExePath.parent_path(), ec).wstring();
		if (ec) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra OpenDocument: %d - %s", ec.value(), ec.message().c_str());
			return;
		}
		if (exec.empty()) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra OpenDocument Executable path empty, PDF software path: %s, current directory: %s", pdfSoftwarePath.string().c_str(), currentExePath.parent_path().string().c_str());
			return;
		}

		std::wstring args = L"\"" + exec + L"\"" + L" " + L"\"" + pdfPathString + L"\"";
		STARTUPINFO si{};
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi{};
		bool success = CreateProcessW(nullptr, args.data(), nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi);
		if (!success) {
			ret = GetLastError();
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra OpenDocument CreateProcess '%s' failed: 0x%08lx", utf16_to_utf8(args).c_str(), ret);
		} else {
			SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeSumatra spawned '%s'", utf16_to_utf8(args).c_str());
		}
	} else {
		std::wstring ddeCmd = L"[Open(\"" + this->pdfPathString + L"\",0,1,1)]";

		this->ExecuteDDECommand(ddeCmd);
	}
}

void PDFBridgeSumatra::CloseDocument() {
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "%s", "PDFBridgeSumatra CloseDocument");

	this->pdfPathString.clear();

	// Unregister DDE server
	if (this->idInst != 0L) {
		HDDEDATA hDataNameService = DdeNameService(this->idInst, nullptr, nullptr, DNS_UNREGISTER);

		if (hDataNameService == nullptr) {
			UINT ret = DdeGetLastError(this->idInst);
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra CloseDocument DdeNameService failed: 0x%04x", ret);
		}
	}

	// Disconnect from client
	if (this->hConv != nullptr) {
		bool success = DdeDisconnect(this->hConv);
		this->hConv = nullptr;

		if (!success) {
			UINT ret = DdeGetLastError(this->idInst);
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSumatra CloseDocument DdeDisconnect failed: 0x%04x", ret);
		}
	}
}

void PDFBridgeSumatra::DocumentSearch(const std::string &str, bool wholeWordsOnly, bool caseSensitive) {
	if (this->idInst == 0L) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", "PDFBridgeSumatra DocumentSearch DDE not initialized");
	}

	if (this->hConv == nullptr) {
		if (!this->ConnectDDEClient(this->szService, this->szTopic)) {
			return;
		}
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
