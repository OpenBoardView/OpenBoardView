#include "Board.h"
#include <cctype>
#include <cstdlib>

#include <tuple>

struct ParsedPinString {
	std::string non_digit_prefix;
	int digits_value = {};

	ParsedPinString(const std::string &s)
		// convert digits after the prefix to int; if there are no digits atoi would be called on terminating '\0' and safely return 0
		: non_digit_prefix(begin(s), std::find_if(begin(s), end(s), isdigit))
		, digits_value(atoi(&s[non_digit_prefix.size()])) {}
};

std::tuple<std::size_t, std::string, int, int> make_pin_ordering_tuple(const Pin &pin) {
	// The wanted order for BGA pins looks like:
	// A0 A1..A9 A10 A11..A99 A100 A101..B0..Z0..AA0..AZ0..BA0..
	// Information from both `number` and `name` fields is used, but `number` has precedence.
	ParsedPinString number{pin.number};
	ParsedPinString name{pin.name};
	std::string non_digit_text{number.non_digit_prefix.empty() ? name.non_digit_prefix : number.non_digit_prefix};

	// To implement the BGA pins ordering mentioned above return a tuple (used as a comparison key) with the following precedence:
	// text length, text ascii, digits from `number`, digits from `name`
	return {non_digit_text.size(), non_digit_text, number.digits_value, name.digits_value};
}

bool Pin::LessByNumberAndName::operator()(const std::shared_ptr<Pin> &a, const std::shared_ptr<Pin> &b) const {
	// Caller must ensure that shared_ptr arguments are non-nullptr (used only for indirection, not for optionality).
	// This comparison operator MUST satisfy the strict weak ordering definition; otherwise std::sort can crash!
	// Using tuple as keys with their default comparison helps ensure this invariant.
	return make_pin_ordering_tuple(*a) < make_pin_ordering_tuple(*b);
}

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
