#include "PDFBridge.h"

#include <SDL.h>

PDFBridge::~PDFBridge() {
}

bool PDFBridge::HasNewSelection() {
	// Called every frame from the render loop, so stay silent here to avoid log spam.
	return false;
}

std::string PDFBridge::GetSelection() const {
	return {};
}

void PDFBridge::OpenDocument(const PDFFile &pdfFile) {
	SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
	            "PDFBridge: no PDF bridge backend is available on this platform; "
	            "boardview <-> schematic cross-probing is disabled.");
}

void PDFBridge::CloseDocument() {
}

void PDFBridge::DocumentSearch(const std::string &str, bool wholeWordsOnly, bool caseSensitive) {
	SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
	            "PDFBridge: cannot search the schematic for \"%s\" -- no PDF bridge backend "
	            "on this platform.", str.c_str());
}
