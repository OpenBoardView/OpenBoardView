#ifndef _PREFERENCES_BOARDAPPEARANCE_H_
#define _PREFERENCES_BOARDAPPEARANCE_H_

#include "GUI/Config.h"
#include "confparse.h"

namespace Preferences {

class BoardAppearance {
private:
	bool shown = false;
	Confparse &obvconfig;
	Config &config;
public:
	BoardAppearance(Confparse &obvconfig, Config &config);

	void render();
};

} // namespace Preferences

#endif
