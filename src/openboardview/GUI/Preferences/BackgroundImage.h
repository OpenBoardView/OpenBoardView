#ifndef _PREFERENCES_BACKGROUNDIMAGE_H_
#define _PREFERENCES_BACKGROUNDIMAGE_H_

#include "UI/Keyboard/KeyBindings.h"

#include "GUI/BackgroundImage.h"

#include <string>

namespace Preferences {

class BackgroundImage {
private:
	bool shown = false;
	std::vector<std::string> erroredFiles;
	const KeyBindings &keybindings;
	::BackgroundImage &backgroundImage;
	::BackgroundImage backgroundImageCopy{::BackgroundImage::defaultSide};

	void imageSettings(const std::string &name, Image &image);
	void errorPopup();
public:
	BackgroundImage(const KeyBindings &keybindings, ::BackgroundImage &backgroundImage);

	void menuItem();
	void render();
};

} // namespace Preferences

#endif
