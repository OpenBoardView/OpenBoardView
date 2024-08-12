#ifndef _FONTS_H_
#define _FONTS_H_

#include <string>

class Fonts {
public:
	std::string load(std::string customFont, double fontSize);
	std::string reload(std::string customFont, double fontSize);
};

#endif//_FONTS_H_
