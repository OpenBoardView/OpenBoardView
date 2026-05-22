#ifndef _FONTS_H_
#define _FONTS_H_

#include <string>

class Fonts {
public:
	 // Arbritarily chosen small value to attempt not to overflow texture size especially on poor GPU/drivers with e.g., 1024x1024 max (Windows GDI)
	static constexpr const float MAX_FONT_SIZE = 72.0f;

	std::string load(std::string customFont);
	std::string reload(std::string customFont);
};

#endif//_FONTS_H_
