#ifndef _PREFERENCES_BOARDSETTINGS_H_
#define _PREFERENCES_BOARDSETTINGS_H_

#include "UI/Keyboard/KeyBindings.h"

#include "BackgroundImage.h"
#include "PDFFile.h"

#include <string>

namespace Preferences {

class BoardSettings {
private:
	bool shown = false;
	const KeyBindings &keybindings;
	Preferences::BackgroundImage backgroundImagePreferences;
	Preferences::PDFFile pdfFilePreferences;

public:
	BoardSettings(const KeyBindings &keybindings, ::BackgroundImage &backgroundImage, ::PDFFile &pdfFile);

	void menuItem();
	void render();
};

} // namespace Preferences

#endif
