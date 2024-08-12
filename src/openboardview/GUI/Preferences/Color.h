#ifndef _PREFERENCES_COLOR_H_
#define _PREFERENCES_COLOR_H_

#include <cstdint>

#include "GUI/ColorScheme.h"
#include "UI/Keyboard/KeyBindings.h"
#include "confparse.h"

namespace Preferences {

class Color {
private:
	bool shown = false;
	KeyBindings	&keyBindings;
	Confparse &obvconfig;
	ColorScheme &colorScheme;

	void ColorPreferencesItem(const char *label,
						   int label_width,
						   const char *butlabel,
						   const char *conflabel,
						   int var_width,
						   uint32_t *c);
public:
	Color(KeyBindings &keyBindings, Confparse &obvconfig, ColorScheme &colorScheme);

	void menuItem();
	bool render();
};

} // namespace Preferences

#endif
