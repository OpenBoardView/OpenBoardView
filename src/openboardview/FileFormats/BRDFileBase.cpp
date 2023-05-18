#include "BRDFileBase.h"

#include "utf8/utf8.h"
#include <cstdint>
#include <cstring>

// from stb.h
void stringfile(char *buffer, std::vector<char*> &lines) {
	char *s;
	size_t count, i;

	// two passes through: first time count lines, second time set them
	for (i = 0; i < 2; ++i) {
		s                   = buffer;
		if (i == 1) lines[0] = s;
		count               = 1; // was '1',  but C arrays are 0-indexed
		while (*s) {
			if (*s == '\n' || *s == '\r') {

				// If this is the 2nd pass, then terminate the line at the first line break char
				if (i == 1) *s = 0;

				s++; // next char

				// if the termination is a CRLF combo, then jump to the next char
				if ((*s == '\r') || (*s == '\n')) s++;

				// if the char is valid (first after line break), set up the next item in the line array
				if (*s) { // it's not over yet
					if (i == 1) {
						lines[count] = s;
						//					  list[count+1] = NULL;
						// fprintf(stdout,"%s\n",list[count]);
					}
					++count;
				}
			}
			s++;
		} // while s

		// Generate the required array to hold all the line starting points
		if (i == 0) {
			lines.resize(count);
		}
	}
}

char *fix_to_utf8(char *s, char **arena, char *arena_end) {
	if (!utf8valid(s)) {
		return s;
	}
	char *p     = *arena;
	char *begin = p;
	while (*s) {
		uint32_t c = (uint8_t)*s;
		if (c < 0x80) {
			if (p + 1 >= arena_end) goto done;
			*p++ = c;
		} else {
			if (p + 2 >= arena_end) goto done;
			*p++ = 0xc0 | (c >> 6);
			*p++ = 0x80 | (c & 0x3f);
		}
		++s;
	}
	if (p + 1 >= arena_end) goto done;
	*p++ = 0;
done:
	*arena = p;
	return begin;
}

struct PinNameParseInfo {
	int non_digit_prefix_len = 0;
	int digits_value         = 0;
};

static PinNameParseInfo pin_name_parse(const BRDPin &pin) {
	PinNameParseInfo result = {};
	if (pin.name != nullptr) {
		for (;;) {
			char next = pin.name[result.non_digit_prefix_len];
			if (next == '\0' || (next >= '0' && next <= '9')) {
				break;
			}
			++result.non_digit_prefix_len;
		}
		result.digits_value = strtod(pin.name + result.non_digit_prefix_len, nullptr);
	}
	return result;
}

bool BRDPin::LessByPartAndNumberAndName::operator()(const BRDPin &a, const BRDPin &b) const {
	// This comparison operator MUST satisfy strict weak ordering definition; otherwise std::sort can crash!
	// At first compare parts
	if (a.part != b.part) {
		return a.part < b.part;
	}
	// for equal parts compare pin number
	if (a.snum != nullptr && b.snum != nullptr) {
		int anum = strtod(a.snum, nullptr);
		int bnum = strtod(b.snum, nullptr);
		if (anum != bnum) {
			return anum < bnum;
		}
	}
	// for equal parts and numbers compare pin name, supporting strings like AA51. The order is like:
	// A0 A1..A9 A10 A11..A99 A100 A101..B0..Z0..AA0..AZ0..BA0..
	PinNameParseInfo a_name = pin_name_parse(a);
	PinNameParseInfo b_name = pin_name_parse(b);

	// compare non-digit prefixes lengths
	if (a_name.non_digit_prefix_len != b_name.non_digit_prefix_len) {
		return a_name.non_digit_prefix_len < b_name.non_digit_prefix_len;
	}

	// compare non-digit prefixes of equal length if non-empty
	if (a_name.non_digit_prefix_len > 0) {
		int prefix_cmp_result = memcmp(a.name, b.name, a_name.non_digit_prefix_len);
		if (prefix_cmp_result != 0) {
			return prefix_cmp_result < 0;
		}
	}
	return a_name.digits_value < b_name.digits_value;
}

void BRDFileBase::AddNailsAsPins() {
	for (auto &nail : nails) {
		BRDPin pin;
		pin.pos = nail.pos;
		if (nail.side == BRDPartMountingSide::Both) {
			pin.part = parts.size();
			pin.side = BRDPinSide::Both;
		} else if (nail.side == BRDPartMountingSide::Top) {
			pin.part = parts.size();
			pin.side = BRDPinSide::Top;
		} else {
			pin.part  = parts.size() - 1;
			pin.side = BRDPinSide::Bottom;
		}
		pin.probe = nail.probe;
		pin.net   = nail.net;
		pins.push_back(pin);
	}
}
