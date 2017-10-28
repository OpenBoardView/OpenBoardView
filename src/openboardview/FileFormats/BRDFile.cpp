#include "BRDFile.h"

#include "utf8/utf8.h"
#include "utils.h"
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>

// Header for recognizing a BRD file
decltype(BRDFile::signature) constexpr BRDFile::signature;

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

/*
 * Returns true if the file format seems to be BRD.
 * Uses std::string::find() on a std::string rather than strstr() on the buffer because the latter expects a null-terminated string.
 */
bool BRDFile::verifyFormat(std::vector<char> &buf) {
	if (buf.size() < signature.size()) return false; // C++14 implements a safer std::equal where this is not needed
	if (std::equal(signature.begin(), signature.end(), buf.begin(), [](const uint8_t &i, const char &j) {
		    return i == reinterpret_cast<const uint8_t &>(j);
		}))
		return true;
	return find_str_in_buf("str_length:", buf) && find_str_in_buf("var_data:", buf);
}

BRDFile::BRDFile(std::vector<char> &buf) {
	auto buffer_size = buf.size();
	ENSURE(buffer_size > 4);
	size_t file_buf_size = 3 * (1 + buffer_size);
	file_buf             = (char *)calloc(1, file_buf_size);
	ENSURE(file_buf != nullptr);

	std::copy(buf.begin(), buf.end(), file_buf);
	file_buf[buffer_size] = 0;
	// This is for fixing degenerate utf8
	char *arena     = &file_buf[buffer_size + 1];
	char *arena_end = file_buf + file_buf_size - 1;
	*arena_end      = 0;

	// decode the file if it appears to be encoded:
	static const uint8_t encoded_header[] = {0x23, 0xe2, 0x63, 0x28};
	if (!memcmp(file_buf, encoded_header, 4)) {
		for (size_t i = 0; i < buffer_size; i++) {
			char x = file_buf[i];
			if (!(x == '\r' || x == '\n' || !x)) {
				int c = x;
				x     = ~(((c >> 6) & 3) | (c << 2));
			}
			file_buf[i] = x;
		}
	}

	int current_block = 0;
	std::vector<char*> lines;
	stringfile(file_buf, lines);

	for (char *line : lines) {
		while (isspace((uint8_t)*line)) line++;
		if (!line[0]) continue;
		if (!strcmp(line, "str_length:")) {
			current_block = 1;
			continue;
		}
		if (!strcmp(line, "var_data:")) {
			current_block = 2;
			continue;
		}
		if (!strcmp(line, "Format:")) {
			current_block = 3;
			continue;
		}
		if (!strcmp(line, "Parts:")) {
			current_block = 4;
			continue;
		}
		if (!strcmp(line, "Pins:")) {
			current_block = 5;
			continue;
		}
		if (!strcmp(line, "Nails:")) {
			current_block = 6;
			continue;
		}

		char *p = line;
		char *s;
		unsigned int tmp = 0;

		switch (current_block) {
			case 2: { // var_data
				num_format = READ_UINT();
				num_parts  = READ_UINT();
				num_pins   = READ_UINT();
				num_nails  = READ_UINT();
			} break;
			case 3: { // Format
				ENSURE(format.size() < num_format);
				BRDPoint fmt;
				fmt.x = strtol(p, &p, 10);
				fmt.y = strtol(p, &p, 10);
				format.push_back(fmt);
			} break;
			case 4: { // Parts
				ENSURE(parts.size() < num_parts);
				BRDPart part;
				part.name      = READ_STR();
				tmp            = READ_UINT(); // Type and layer, actually.
				part.part_type = (tmp & 0xc) ? BRDPartType::SMD : BRDPartType::ThroughHole;
				if (tmp == 1 || (4 <= tmp && tmp < 8)) part.mounting_side = BRDPartMountingSide::Top;
				if (tmp == 2 || (8 <= tmp)) part.mounting_side            = BRDPartMountingSide::Bottom;
				part.end_of_pins                                          = READ_UINT();
				ENSURE(part.end_of_pins <= num_pins);
				parts.push_back(part);
			} break;
			case 5: { // Pins
				ENSURE(pins.size() < num_pins);
				BRDPin pin;
				pin.pos.x = READ_INT();
				pin.pos.y = READ_INT();
				pin.probe = READ_INT(); // Can be negative (-99)
				pin.part  = READ_UINT();
				ENSURE(pin.part <= num_parts);
				pin.net = READ_STR();
				pins.push_back(pin);
			} break;
			case 6: { // Nails
				ENSURE(nails.size() < num_nails);
				BRDNail nail;
				nail.probe = READ_UINT();
				nail.pos.x = READ_INT();
				nail.pos.y = READ_INT();
				nail.side  = READ_UINT();
				nail.net   = READ_STR();
				nails.push_back(nail);
			} break;
		}
	}
	valid = current_block != 0;
}
