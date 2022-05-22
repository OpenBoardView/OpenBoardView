#include "PDFBridge.h"

PDFBridge::~PDFBridge() {
}

bool PDFBridge::HasNewSelection() {
	return false;
}

std::string PDFBridge::GetSelection() const {
	return {};
}

void PDFBridge::OpenDocument(const PDFFile &pdfFile) {
}

void PDFBridge::CloseDocument() {
}

void PDFBridge::DocumentSearch(const std::string &str, bool wholeWordsOnly, bool caseSensitive) {
}
