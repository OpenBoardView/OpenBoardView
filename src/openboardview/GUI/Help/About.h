#ifndef _HELP_ABOUT_H_
#define _HELP_ABOUT_H_

#include <string>

#include "UI/Keyboard/KeyBindings.h"

namespace Help {

class About {
private:
	bool shown = false;
	static const std::string OBV_LICENSE_TEXT_NONWRAPPED;

	KeyBindings	&keybindings;

	static std::string removeSingleLineFeed(std::string str);
public:
	About(KeyBindings &keybindings);

	void menuItem();
	void render();
};

} // namespace Help
//
#endif
