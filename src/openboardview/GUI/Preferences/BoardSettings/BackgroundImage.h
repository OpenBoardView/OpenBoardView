#ifndef _PREFERENCES_BACKGROUNDIMAGE_H_
#define _PREFERENCES_BACKGROUNDIMAGE_H_

#include "GUI/BackgroundImage.h"

#include <string>
#include <vector>

namespace Preferences {

class BackgroundImage {
private:
	std::vector<std::string> erroredFiles;
	::BackgroundImage &backgroundImage;
	::BackgroundImage backgroundImageCopy{::BackgroundImage::defaultSide};

	void imageSettings(const std::string &name, Image &image);
	void errorPopup();
public:
	BackgroundImage(::BackgroundImage &backgroundImage);

	void render(bool shown);
	void save();
	void cancel();
	void clear();
};

} // namespace Preferences

#endif
