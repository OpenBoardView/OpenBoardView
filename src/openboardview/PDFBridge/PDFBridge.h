#ifndef _PDFBRIDGE_H_
#define _PDFBRIDGE_H_

#include "filesystem_impl.h"

class PDFBridge {
public:
	virtual ~PDFBridge();

	virtual void OpenDocument(const filesystem::path &pdfPath) ;
	virtual void CloseDocument();
	virtual void DocumentSearch(const std::string &str, bool wholeWordsOnly, bool caseSensitive);
	virtual bool HasNewSelection();
	virtual std::string GetSelection() const;
};

#endif//_PDFBRIDGE_H_
