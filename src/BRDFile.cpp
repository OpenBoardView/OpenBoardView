#include "BRDFile.h"

#include "utf8.h"
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>

// from stb.h -- return value must be freed
char **stringfile(char *buffer) {
	char **list = nullptr, *s;
	size_t count, i;

	// two passes through: first time count lines, second time set them
	for (i = 0; i < 2; ++i) {
		s = buffer;
		if (i == 1)
			list[0] = s;
		count = 1;
		while (*s) {
			if (*s == '\n' || *s == '\r') {
				// detect if both cr & lf are together
				int crlf = (s[0] + s[1]) == ('\n' + '\r');
				if (i == 1)
					*s = 0;
				if (crlf)
					++s;
				if (s[1]) { // it's not over yet
					if (i == 1)
						list[count] = s + 1;
					++count;
				}
			}
			++s;
		}
		if (i == 0) {
			list = (char **)malloc(sizeof(*list) * (count + 2));
			if (!list)
				return NULL;
			list[count] = 0;
		}
	}
	return list;
}

char *fix_to_utf8(char *s, char **arena, char *arena_end) {
	if (!utf8valid(s)) {
		return s;
	}
	char *p = *arena;
	char *begin = p;
	while (*s) {
		uint32_t c = (uint8_t)*s;
		if (c < 0x80) {
			if (p + 1 >= arena_end)
				goto done;
			*p++ = c;
		} else {
			if (p + 2 >= arena_end)
				goto done;
			*p++ = 0xc0 | (c >> 6);
			*p++ = 0x80 | (c & 0x3f);
		}
		++s;
	}
	if (p + 1 >= arena_end)
		goto done;
	*p++ = 0;

// GOTO done
done:
	*arena = p;
	return begin;
}

BRDFile::BRDFile(const char *buf, size_t buffer_size) {
	memset(this, 0, sizeof(*this));

	#define ENSURE(X) \
	  assert(X);      \
	  if (!(X))       \
	    return;

	ENSURE(buffer_size > 4);
	size_t file_buf_size = 3 * (1 + buffer_size);
	file_buf = (char *)malloc(file_buf_size);
	memcpy(file_buf, buf, buffer_size);
	file_buf[buffer_size] = 0;
	// This is for fixing degenerate utf8
	char *arena = &file_buf[buffer_size + 1];
	char *arena_end = file_buf + file_buf_size - 1;
	*arena_end = 0;

	// decode the file if it appears to be encoded:
	static const uint8_t encoded_header[] = {0x23, 0xe2, 0x63, 0x28};
	if (!memcmp(file_buf, encoded_header, 4)) {
		for (size_t i = 0; i < buffer_size; i++) {
			char x = file_buf[i];
			if (!(x == '\r' || x == '\n' || !x)) {
				int c = x;
				x = ~(((c >> 6) & 3) | (c << 2));
			}
			file_buf[i] = x;
		}
	}

	int current_block = 0;
	num_format = 0;
	num_parts = 0;
	num_pins = 0;
	num_nails = 0;
	int format_idx = 0;
	int parts_idx = 0;
	int pins_idx = 0;
	int nails_idx = 0;
	char **lines = stringfile(file_buf);
	if (!lines)
		return;
	char **lines_begin = lines;

	#undef ENSURE
	#define ENSURE(X)      \
	  assert(X);           \
	  if (!(X)) {          \
	    free(lines_begin); \
	    return;            \
	  }

	while (*lines) {
		char *line = *lines;
		++lines;

		while (isspace((uint8_t)*line))
			line++;
		if (!line[0])
			continue;
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
			LOAD_INT(part.type);
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
}

// Ensure this files #defines dont bleed out
#undef ENSURE
