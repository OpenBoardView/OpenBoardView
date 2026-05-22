#include "Board.h"

std::vector<const std::string *> Component::searchableStringDetails() const {
	return {&mfgcode};
}

std::vector<const std::string *> Net::searchableStringDetails() const {
	std::vector<const std::string *> result = {};
	for (const auto &pin : pins) {
		result.push_back(&pin->name);
		if (pin->number != pin->name) {
			result.push_back(&pin->number);
		}
	}
	return result;
}
