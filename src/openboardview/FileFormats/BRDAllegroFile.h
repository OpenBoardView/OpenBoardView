#pragma once

#include "BRDFileBase.h"

struct BRDAllegroFile : public BRDFileBase {
	BRDAllegroFile(std::vector<char> &/*buf*/) {
		valid = false;
		error_msg = "Allegro format is not supported. Please use Allegro® FREE Physical Viewer.";
	}

	static bool verifyFormat(std::vector<char> &buf) {
		// Allegro files contain the string "allv" + version number ("15", "16", …) at offset 0xf8
		return buf.size() >= 0xfb && std::equal(buf.begin() + 0xf8, buf.begin() + 0xfc, "allv");
	}
};
