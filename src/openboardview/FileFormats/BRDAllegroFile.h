#pragma once

#include "BRDFileBase.h"

struct BRDAllegroFile : public BRDFileBase {
	BRDAllegroFile(std::vector<char> &/*buf*/) {
		valid = false;
		error_msg = "Allegro format is not supported. Please use Allegro® FREE Physical Viewer.";
	}

	static bool verifyFormat(std::vector<char> &buf) {
		// Allegro files contain the string "all" or "vie" + version number ("v15", "v16", …) at offset 0xf8
		return buf.size() >= 0xfa
				&& (std::equal(buf.begin() + 0xf8, buf.begin() + 0xfb, "all")
					|| std::equal(buf.begin() + 0xf8, buf.begin() + 0xfb, "vie"));
	}
};
