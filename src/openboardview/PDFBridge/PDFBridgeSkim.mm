#include "PDFBridgeSkim.h"

#ifdef __APPLE__

#import <Foundation/Foundation.h>
#include <SDL.h>

PDFBridgeSkim::PDFBridgeSkim() {}
PDFBridgeSkim::~PDFBridgeSkim() {}

std::string PDFBridgeSkim::escapeForAppleScript(const std::string &s) {
	std::string result;
	for (char c : s) {
		if (c == '"') result += "\\\"";
		else result += c;
	}
	return result;
}

void PDFBridgeSkim::runScript(const std::string &script) {
	// Write script to temp file and run with osascript
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

	// Aynı terme tekrar basılınca son bulunan sayfadan sonrasından ara
	// Farklı terme basılınca baştan başla
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
		// Sona gelindi, başa dön
		lastFoundPage = 0;
		DocumentSearch(str, wholeWordsOnly, caseSensitive);
		return;
	}
	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeSkim: searching for '%s' page=%s", str.c_str(), result.c_str());
}

bool PDFBridgeSkim::HasNewSelection() {
	if (currentPdfPath.empty()) return false;

	// Poll at most every 2 seconds to avoid freezing the UI
	auto now = std::chrono::steady_clock::now();
	if (std::chrono::duration_cast<std::chrono::seconds>(now - lastPollTime).count() < 2)
		return false;
	lastPollTime = now;

	// Poll Skim's current text selection for reverse search
	std::string script =
		"tell application \"Skim\"\n"
		"    if (count of documents) > 0 then\n"
		"        set sel to selection of front document\n"
		"        if sel is not missing value then\n"
		"            return string of sel\n"
		"        end if\n"
		"    end if\n"
		"    return \"\"\n"
		"end tell";

	std::string polled = runScriptResult(script);

	if (!polled.empty() && polled != lastPolledSelection) {
		lastPolledSelection = polled;
		selection = polled;
		hasNewSelection = true;
		return true;
	}

	return false;
}

std::string PDFBridgeSkim::GetSelection() const {
	return selection;
}

#endif // __APPLE__
