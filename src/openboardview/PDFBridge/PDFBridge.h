#ifndef _PDFBRIDGE_H_
#define _PDFBRIDGE_H_

#include "filesystem_impl.h"

class PDFBridge {
private:
	const filesystem::path pdfPath;
public:
	virtual ~PDFBridge() {}

	virtual void OpenDocument(const filesystem::path &pdfPath) = 0;
	virtual void CloseDocument() = 0;
	virtual void DocumentSearch(const std::string &str, bool wholeWordsOnly, bool caseSensitive) = 0;
	virtual bool HasNewSelection() = 0;
	virtual std::string GetSelection() const = 0;
};

#endif//_PDFBRIDGE_H_
