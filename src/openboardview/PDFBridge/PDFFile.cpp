#include "platform.h"

#include "PDFFile.h"

#include "confparse.h"

PDFFile::PDFFile(PDFBridge &pdfBridge) : pdfBridge(&pdfBridge) {
}

void PDFFile::reload() {
	if (this->pdfBridge == nullptr) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", "PDFFile Could not reload: pdfBridge == nullptr");
		return;
	}
	this->pdfBridge->CloseDocument();
	this->pdfBridge->OpenDocument(*this);
}

void PDFFile::close() {
	if (this->pdfBridge == nullptr) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", "PDFFile Could not close: pdfBridge == nullptr");
		return;
	}
	this->pdfBridge->CloseDocument();
}

void PDFFile::loadFromConfig(const filesystem::path &filepath) {
	configFilepath = filepath; // save filepath for latter use with writeToConfig

	// Use boardview file path as default PDF file path, with ext replaced with .pdf
	path = filepath;
	path.replace_extension("pdf");

	if (!filesystem::exists(filepath)) // Config file doesn't exist, do not attempt to read or write it and load images
		return;

	auto configDir = filesystem::weakly_canonical(filepath).parent_path();

	Confparse confparse{};
	confparse.Load(filepath);


	std::string pdfFilePathStr{confparse.ParseStr("PDFFilePath", "")};
	if (!pdfFilePathStr.empty())
		path = configDir/filesystem::u8path(pdfFilePathStr);

	writeToConfig(filepath);
}

void PDFFile::writeToConfig(const filesystem::path &filepath) {
	if (filepath.empty()) // No destination file to save to
		return;

	std::error_code ec;
	auto confparse = Confparse{};
	confparse.Load(filepath);

	auto configDir = filesystem::weakly_canonical(filepath).parent_path();

	if (!path.empty()) {
		auto pdfFilePath = filesystem::relative(path, configDir, ec);
		if (ec) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error writing PDF file path: %d - %s", ec.value(), ec.message().c_str());
		} else {
			confparse.WriteStr("PDFFilePath", pdfFilePath.string().c_str());
		}
	}
}

filesystem::path PDFFile::getPath() const {
	return this->path;
}
