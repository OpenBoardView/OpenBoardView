#include "PDFBridgeSkim.h"

#ifdef __APPLE__

#import <Foundation/Foundation.h>
#include <SDL.h>
#include <cctype>

// ---------- helpers ----------

std::string PDFBridgeSkim::escapeForAppleScript(const std::string &s) {
	std::string result;
	for (char c : s) {
		if (c == '"') result += "\\\"";
		else result += c;
	}
	return result;
}

void PDFBridgeSkim::runScript(const std::string &script) {
	std::string tmpFile = "/tmp/obv_skim_script.applescript";
	FILE *f = fopen(tmpFile.c_str(), "w");
	if (!f) return;
	fwrite(script.c_str(), 1, script.size(), f);
	fclose(f);
	std::string cmd = "osascript " + tmpFile + " &>/dev/null &";
	system(cmd.c_str());
}

std::string PDFBridgeSkim::runScriptResult(const std::string &script) {
	std::string tmpFile = "/tmp/obv_skim_query.applescript";
	FILE *f = fopen(tmpFile.c_str(), "w");
	if (!f) return "";
	fwrite(script.c_str(), 1, script.size(), f);
	fclose(f);
	std::string cmd = "osascript " + tmpFile + " 2>/dev/null";
	FILE *pipe = popen(cmd.c_str(), "r");
	if (!pipe) return "";
	char buf[512];
	std::string result;
	while (fgets(buf, sizeof(buf), pipe)) result += buf;
	pclose(pipe);
	if (!result.empty() && result.back() == '\n') result.pop_back();
	return result;
}

// ---------- background polling thread ----------

// Trim whitespace from both ends
static std::string trimWS(const std::string &s) {
	size_t start = s.find_first_not_of(" \t\n\r");
	if (start == std::string::npos) return "";
	size_t end = s.find_last_not_of(" \t\n\r");
	return s.substr(start, end - start + 1);
}


void PDFBridgeSkim::pollLoop() {
	std::string lastSeen;

	while (pollRunning.load()) {
		// 1) Check the Automator request file first (instant, no AppleScript cost)
		{
			const char *reqFile = "/tmp/obv_search_request.txt";
			FILE *f = fopen(reqFile, "r");
			if (f) {
				char buf[512] = {};
				fgets(buf, sizeof(buf), f);
				fclose(f);
				remove(reqFile);
				std::string req = trimWS(std::string(buf));
				if (!req.empty() && req != lastSeen) {
					lastSeen = req;
					std::lock_guard<std::mutex> lk(selectionMutex);
					pendingSelection = req;
					selectionReady.store(true);
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(200));
				continue;
			}
		}

		// 2) Poll Skim's current text selection via AppleScript
		std::string script =
			"tell application \"Skim\"\n"
			"    if (count of documents) > 0 then\n"
			"        set sel to selection of front document\n"
			"        if sel is not missing value then\n"
			"            set txt to \"\"\n"
			"            repeat with s in sel\n"
			"                set txt to txt & (s as string)\n"
			"            end repeat\n"
			"            return txt\n"
			"        end if\n"
			"    end if\n"
			"    return \"\"\n"
			"end tell";

		std::string polled = trimWS(runScriptResult(script));

		if (!polled.empty() && polled != lastSeen) {
			lastSeen = polled;
			std::lock_guard<std::mutex> lk(selectionMutex);
			pendingSelection = polled;
			selectionReady.store(true);
		} else if (polled.empty()) {
			lastSeen = ""; // reset so next selection triggers even if same text
		}

		// Poll every 500 ms
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

// ---------- lifecycle ----------

PDFBridgeSkim::PDFBridgeSkim() {
	pollRunning.store(true);
	pollThread = std::thread(&PDFBridgeSkim::pollLoop, this);
}

PDFBridgeSkim::~PDFBridgeSkim() {
	pollRunning.store(false);
	if (pollThread.joinable()) pollThread.detach(); // don't block — thread dies with process

	// Close only the documents that OBV opened (synchronous)
	for (const auto &path : openedPdfPaths) {
		std::string docName = filesystem::path(path).filename().string();
		std::string escapedDoc = escapeForAppleScript(docName);
		std::string script =
			"tell application \"Skim\"\n"
			"    try\n"
			"        close document \"" + escapedDoc + "\"\n"
			"    end try\n"
			"end tell";
		runScriptResult(script);
	}
}

// ---------- public API ----------

void PDFBridgeSkim::OpenDocument(const PDFFile &pdfFile) {
	auto pdfPath = pdfFile.getPath();
	if (!filesystem::exists(pdfPath)) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSkim: PDF file does not exist: %s",
			pdfPath.generic_string().c_str());
		return;
	}

	currentPdfPath = filesystem::canonical(pdfPath).string();
	std::string escaped = escapeForAppleScript(currentPdfPath);

	std::string script =
		"tell application \"Skim\"\n"
		"    activate\n"
		"    open POSIX file \"" + escaped + "\"\n"
		"end tell";

	runScript(script);
	openedPdfPaths.push_back(currentPdfPath);
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeSkim: opened %s", currentPdfPath.c_str());
}

void PDFBridgeSkim::CloseDocument() {
	currentPdfPath = "";
}

void PDFBridgeSkim::DocumentSearch(const std::string &str, bool wholeWordsOnly, bool caseSensitive) {
	if (currentPdfPath.empty()) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSkim: no document open");
		return;
	}

	std::string escaped = escapeForAppleScript(str);

	std::string docName = filesystem::path(currentPdfPath).filename().string();
	std::string escapedDoc = escapeForAppleScript(docName);
	std::string caseSens = caseSensitive ? " case sensitive search yes" : "";

	bool sameTerm = (str == lastSearchTerm);
	lastSearchTerm = str;

	std::string fromClause;
	if (sameTerm && lastFoundPage > 0) {
		fromClause = " from character 1 of page " + std::to_string(lastFoundPage + 1) + " of doc";
	} else {
		fromClause = " from character 1 of page 1 of doc";
		lastFoundPage = 0;
	}

	std::string script =
		"tell application \"Skim\"\n"
		"    activate\n"
		"    set doc to document \"" + escapedDoc + "\"\n"
		"    set r to find doc text \"" + escaped + "\"" + fromClause + caseSens + "\n"
		"    if (count of r) > 0 then\n"
		"        set selection of doc to r\n"
		"        go doc to item 1 of r\n"
		"        return index of current page of doc\n"
		"    else\n"
		"        return 0\n"
		"    end if\n"
		"end tell";

	std::string result = runScriptResult(script);
	if (!result.empty() && result != "0") {
		try { lastFoundPage = std::stoi(result); } catch (...) {}
	} else if (result == "0" && sameTerm && lastFoundPage > 0) {
		lastFoundPage = 0;
		DocumentSearch(str, wholeWordsOnly, caseSensitive);
		return;
	}
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeSkim: searching for '%s' page=%s", str.c_str(), result.c_str());
}

bool PDFBridgeSkim::HasNewSelection() {
	if (!selectionReady.load()) return false;
	std::lock_guard<std::mutex> lk(selectionMutex);
	selection = pendingSelection;
	selectionReady.store(false);
	return true;
}

std::string PDFBridgeSkim::GetSelection() const {
	return selection;
}

#endif // __APPLE__
