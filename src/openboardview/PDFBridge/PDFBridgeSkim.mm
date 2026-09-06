#include "PDFBridgeSkim.h"

#ifdef ENABLE_PDFBRIDGE_SKIM

#import <Foundation/Foundation.h>

#include <SDL.h>

#include <algorithm>
#include <chrono>

#include "filesystem_impl.h"

namespace {

// Escape a string for embedding inside an AppleScript double-quoted string literal.
std::string escapeForAppleScript(const std::string &in) {
	std::string out;
	out.reserve(in.size());
	for (char c : in) {
		if (c == '\\' || c == '"')
			out.push_back('\\');
		out.push_back(c);
	}
	return out;
}

NSString *toNSString(const std::string &in) {
	NSString *s = [NSString stringWithUTF8String:in.c_str()];
	return s != nil ? s : @"";
}

// Compile and run an AppleScript source string via NSAppleScript, returning the script's
// string result ("" on error or no result). NSAppleScript is main-thread affine, so this
// must only be called from the render/main thread (which is where OBV invokes the bridge).
std::string runAppleScript(NSString *source) {
	@autoreleasepool {
		NSAppleScript *script = [[NSAppleScript alloc] initWithSource:source];
		if (script == nil) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSkim: failed to compile AppleScript");
			return {};
		}

		NSDictionary *errorInfo = nil;
		NSAppleEventDescriptor *result = [script executeAndReturnError:&errorInfo];

		if (errorInfo != nil) {
			NSNumber *errNum = errorInfo[NSAppleScriptErrorNumber];
			NSString *errMsg = errorInfo[NSAppleScriptErrorMessage];
			if (errNum != nil && [errNum intValue] == errAEEventNotPermitted) {
				// This is the silent-failure trap #285 hit: without automation consent the
				// Apple Event is refused. Surface it instead of failing quietly.
				SDL_LogError(SDL_LOG_CATEGORY_ERROR,
				             "PDFBridgeSkim: automation permission denied. Allow OpenBoardView "
				             "to control Skim under System Settings > Privacy & Security > "
				             "Automation, then try again.");
			} else {
				SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSkim AppleScript error: %s",
				             errMsg != nil ? [errMsg UTF8String] : "unknown");
			}
			return {};
		}

		if (result != nil && [result stringValue] != nil)
			return std::string([[result stringValue] UTF8String]);
		return {};
	}
}

// Wrap an operation in a script that resolves `theDoc` to our PDF -- matching an already
// open document by path, or opening it in Skim on demand. This keeps every operation
// robust to the user having closed the window and to other PDFs being open (so we never
// act on the wrong `document 1`). `body` is AppleScript operating on `theDoc`.
NSString *scriptForDocument(const std::string &appName, const std::string &pdfPath, NSString *body) {
	return [NSString stringWithFormat:
		@"tell application \"%@\"\n"
		 "\tactivate\n"
		 "\tset thePath to \"%@\"\n"
		 "\tset theDoc to missing value\n"
		 "\trepeat with d in documents\n"
		 "\t\tif (path of d) is thePath then\n"
		 "\t\t\tset theDoc to d\n"
		 "\t\t\texit repeat\n"
		 "\t\tend if\n"
		 "\tend repeat\n"
		 "\tif theDoc is missing value then\n"
		 "\t\topen POSIX file thePath\n"
		 "\t\trepeat with d in documents\n"
		 "\t\t\tif (path of d) is thePath then\n"
		 "\t\t\t\tset theDoc to d\n"
		 "\t\t\t\texit repeat\n"
		 "\t\t\tend if\n"
		 "\t\tend repeat\n"
		 "\tend if\n"
		 "\tif theDoc is missing value then error \"could not open document\"\n"
		 "%@\n"
		 "end tell",
		toNSString(escapeForAppleScript(appName)),
		toNSString(escapeForAppleScript(pdfPath)),
		body];
}

// Build the read-only script used by the reverse-search poll: find our document by path
// (WITHOUT opening it or activating Skim -- a background poll must never steal focus or
// resurrect a window the user closed) and return the current selection's text, or "".
NSString *selectionQueryScript(const std::string &appName, const std::string &pdfPath) {
	return [NSString stringWithFormat:
		@"tell application \"%@\"\n"
		 "\tset thePath to \"%@\"\n"
		 "\tset theDoc to missing value\n"
		 "\trepeat with d in documents\n"
		 "\t\tif (path of d) is thePath then\n"
		 "\t\t\tset theDoc to d\n"
		 "\t\t\texit repeat\n"
		 "\t\tend if\n"
		 "\tend repeat\n"
		 "\tif theDoc is missing value then return \"\"\n"
		 "\ttry\n"
		 "\t\treturn (obtain text for (selection of theDoc)) as string\n"
		 "\ton error\n"
		 "\t\treturn \"\"\n"
		 "\tend try\n"
		 "end tell",
		toNSString(escapeForAppleScript(appName)),
		toNSString(escapeForAppleScript(pdfPath))];
}

// Run an AppleScript via the `osascript` subprocess and return its trimmed stdout. Used
// only from the background poll thread: NSAppleScript is main-thread affine, whereas a
// subprocess is safe to drive from any thread. TCC attributes the Apple Events to the
// responsible process (OpenBoardView), so the app's existing automation grant covers it.
std::string runOsascript(NSString *source) {
	@autoreleasepool {
		NSTask *task = [[NSTask alloc] init];
		task.launchPath = @"/usr/bin/osascript";
		task.arguments = @[ @"-e", source ];
		NSPipe *outPipe = [NSPipe pipe];
		task.standardOutput = outPipe;
		task.standardError = [NSPipe pipe]; // swallow errors; poll stays quiet to avoid log spam

		@try {
			[task launch];
		} @catch (NSException *e) {
			return {};
		}

		// Read before waiting to avoid deadlocking on a full pipe buffer.
		NSData *data = [[outPipe fileHandleForReading] readDataToEndOfFile];
		[task waitUntilExit];

		NSString *out = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
		if (out == nil)
			return {};
		out = [out stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
		return std::string([out UTF8String]);
	}
}

} // namespace

PDFBridgeSkim::PDFBridgeSkim(Confparse &obvConfig) {
	appName = obvConfig.ParseStr("pdfSkimApplication", "Skim");
	pollIntervalMs = obvConfig.ParseInt("pdfSkimReverseSearchPollMs", 300); // <= 0 disables reverse search
}

PDFBridgeSkim::~PDFBridgeSkim() {
	stopPolling();
}

void PDFBridgeSkim::OpenDocument(const PDFFile &pdfFile) {
	stopPolling(); // stop any poll bound to a previous document before rebinding pdfPath

	auto path = pdfFile.getPath();

	if (!filesystem::exists(path)) { // PDF file does not exist, do not attempt to load
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSkim: PDF file does not exist");
		return;
	}

	pdfPath = filesystem::canonical(path).string();

	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeSkim OpenDocument: %s", pdfPath.c_str());

	// Empty body: just ensure the document is open in Skim.
	runAppleScript(scriptForDocument(appName, pdfPath, @""));

	startPolling(); // begin watching Skim's selection for reverse search
}

void PDFBridgeSkim::CloseDocument() {
	stopPolling(); // joins the poll thread before we touch pdfPath/selection

	// Leave the document open in Skim (the Evince backend likewise does not force-close);
	// just drop our reference so a later search reports "no document" rather than acting on
	// a stale one.
	pdfPath.clear();

	std::lock_guard<std::mutex> lock(selectionMutex);
	currentSelection.clear();
	selectionChanged = false;
}

void PDFBridgeSkim::DocumentSearch(const std::string &str, bool wholeWordsOnly, bool caseSensitive) {
	if (pdfPath.empty()) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PDFBridgeSkim: search attempted without any document open");
		return;
	}

	if (wholeWordsOnly) {
		// Skim's AppleScript 'find' verb has no whole-words option, so this searches as a
		// substring regardless. Noted rather than silently misbehaving.
		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
		             "PDFBridgeSkim: whole-words-only is unsupported by Skim, searching as substring");
	}

	SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "PDFBridgeSkim DocumentSearch: %s", str.c_str());

	NSString *body = [NSString stringWithFormat:
		@"\tset sel to find theDoc text \"%@\" case sensitive search %@\n"
		 "\tif sel is not missing value then\n"
		 "\t\tset selection of theDoc to sel\n"
		 "\t\tgo theDoc to sel\n"
		 "\tend if",
		toNSString(escapeForAppleScript(str)),
		caseSensitive ? @"true" : @"false"];
	runAppleScript(scriptForDocument(appName, pdfPath, body));
}

bool PDFBridgeSkim::HasNewSelection() {
	// Called every frame from the render loop: only compare the cached value, never block.
	std::lock_guard<std::mutex> lock(selectionMutex);
	bool changed = selectionChanged;
	selectionChanged = false;
	return changed;
}

std::string PDFBridgeSkim::GetSelection() const {
	std::lock_guard<std::mutex> lock(selectionMutex);
	return currentSelection;
}

void PDFBridgeSkim::startPolling() {
	if (pollIntervalMs <= 0) // reverse search disabled by config
		return;
	if (pollRunning.exchange(true)) // already running
		return;
	pollThread = std::thread(&PDFBridgeSkim::pollLoop, this);
}

void PDFBridgeSkim::stopPolling() {
	if (!pollRunning.exchange(false))
		return;
	if (pollThread.joinable())
		pollThread.join();
}

void PDFBridgeSkim::pollLoop() {
	NSString *query = selectionQueryScript(appName, pdfPath);

	while (pollRunning) {
		std::string sel = runOsascript(query);

		bool changed = false;
		{
			std::lock_guard<std::mutex> lock(selectionMutex);
			if (sel != currentSelection) {
				currentSelection = sel;
				selectionChanged = true;
				changed = true;
			}
		}

		if (changed) {
			// OBV's render loop skips app.Update() (and thus HandlePDFBridgeSelection) once it
			// has been idle for a while, so a background selection change would not be acted on
			// until the next input event. Push an event to wake the loop and process it promptly.
			SDL_Event ev;
			SDL_zero(ev);
			ev.type = SDL_USEREVENT;
			SDL_PushEvent(&ev);
		}

		// Sleep in small slices so stopPolling() stays responsive.
		for (int slept = 0; slept < pollIntervalMs && pollRunning; slept += 50)
			std::this_thread::sleep_for(std::chrono::milliseconds(std::min(50, pollIntervalMs - slept)));
	}
}

#endif // ENABLE_PDFBRIDGE_SKIM
