#include "BRDFile.h"

#include "utf8/utf8.h"
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>

// Header for recognizing a BRD file
decltype(BRDFile::signature) constexpr BRDFile::signature;

// from stb.h -- return value must be freed
char **stringfile(char *buffer) {
	char **list = nullptr, *s;
	size_t count, i;

	// two passes through: first time count lines, second time set them
	for (i = 0; i < 2; ++i) {
		s                   = buffer;
		if (i == 1) list[0] = s;
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
						list[count] = s;
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
			list = (char **)malloc(sizeof(*list) * (count + 2));
			if (!list) return NULL;
			list[count] = 0;
		}
	}
	return list;
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
bool BRDFile::verifyFormat(const char *buf, size_t buffer_size) {
	if (buffer_size < signature.size()) return false;
	if (std::equal(signature.begin(), signature.end(), (uint8_t *)buf)) return true;
	std::string sbuf(buf, buffer_size); // prevents us from reading beyond the buffer size
	if ((sbuf.find("str_length:") != std::string::npos) && (sbuf.find("var_data:") != std::string::npos)) return true;
	return false;
}

BRDFile::BRDFile(const char *buf, size_t buffer_size) {
//	memset(this, 0, sizeof(*this)); //  old habits must die?
//
//#define ENSURE(X)                                                                                  \
//	assert(X);                                                                                     \
//	if (!(X))                                                                                      \
//		goto FAIL_LABEL;
#define ENSURE(X) \
	assert(X);    \
	if (!(X)) return;

#define FAIL_LABEL fail
	ENSURE(buffer_size > 4);
	size_t file_buf_size = 3 * (1 + buffer_size);
	file_buf             = (char *)malloc(file_buf_size);
	if (!file_buf) return;

	memcpy(file_buf, buf, buffer_size);
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
	num_format        = 0;
	num_parts         = 0;
	num_pins          = 0;
	num_nails         = 0;
	int format_idx    = 0;
	int parts_idx     = 0;
	int pins_idx      = 0;
	int nails_idx     = 0;
	char **lines      = stringfile(file_buf);
	if (!lines) return;
	//		goto fail;
	char **lines_begin = lines;
#undef FAIL_LABEL
#define FAIL_LABEL fail_lines
	while (*lines) {
		char *line = *lines;
		++lines;

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
#define LOAD_INT(var) var = strtol(p, &p, 10)
#define LOAD_STR(var)                            \
	while ((*p) && (isspace((uint8_t)*p))) ++p;  \
	s = p;                                       \
	while ((*p) && (!isspace((uint8_t)*p))) ++p; \
	*p++ = 0;                                    \
	var  = fix_to_utf8(s, &arena, arena_end);
		switch (current_block) {
			case 2: { // var_data
				LOAD_INT(num_format);
				LOAD_INT(num_parts);
				LOAD_INT(num_pins);
				LOAD_INT(num_nails);
				ENSURE(num_format >= 0);
				ENSURE(num_parts >= 0);
				ENSURE(num_pins >= 0);
				ENSURE(num_nails >= 0);
			} break;
			case 3: { // Format
				ENSURE(format_idx < num_format);
				BRDPoint fmt;
				fmt.x = strtol(p, &p, 10);
				fmt.y = strtol(p, &p, 10);
				format.push_back(fmt);
			} break;
			case 4: { // Parts
				ENSURE(parts_idx < num_parts);
				BRDPart part;
				LOAD_STR(part.name);
				LOAD_INT(part.type); // Type, or *layer* ?
				LOAD_INT(part.end_of_pins);
				ENSURE(part.end_of_pins <= num_pins);
				parts.push_back(part);
				parts_idx++;
			} break;
			case 5: { // Pins
				ENSURE(pins_idx < num_pins);
				BRDPin pin;
				LOAD_INT(pin.pos.x);
				LOAD_INT(pin.pos.y);
				LOAD_INT(pin.probe);
				LOAD_INT(pin.part);
				LOAD_STR(pin.net);
				ENSURE(pin.part <= num_parts);
				pins.push_back(pin);
				pins_idx++;
			} break;
			case 6: { // Nails
				ENSURE(nails_idx < num_nails);
				BRDNail nail;
				LOAD_INT(nail.probe);
				LOAD_INT(nail.pos.x);
				LOAD_INT(nail.pos.y);
				LOAD_INT(nail.side);
				LOAD_STR(nail.net);
				nails.push_back(nail);
				nails_idx++;
			} break;
		}
	}
	valid = current_block != 0;
fail_lines:
	free(lines_begin);
fail:;
#undef FAIL_LABEL
#undef ENSURE
}
