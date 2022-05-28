#ifndef _PDFFILE_H_
#define _PDFFILE_H_

#include <array>
#include <string>
#include <glad/glad.h>

#include "imgui/imgui.h"

#include "Renderers/ImGuiRendererSDL.h"
#include "GUI/Image.h"
#include "filesystem_impl.h"

#include "PDFBridge.h"

// Requires forward declaration in order to friend since it's from another namespace
namespace Preferences {
	class PDFFile;
}

class PDFBridge; // Forward declaration to solve circular includes

// Manages background image configuration attached to boardview file and currently shown image depending on current board side
class PDFFile {
private:
	filesystem::path path;
	filesystem::path configFilepath{};

	PDFBridge *pdfBridge = nullptr;
public:
	PDFFile(PDFBridge &pdfBridge);

	void reload();
	void close();

	void loadFromConfig(const filesystem::path &filepath);
	void writeToConfig(const filesystem::path &filepath);
	filesystem::path getPath() const;

	friend Preferences::PDFFile;
};

#endif
