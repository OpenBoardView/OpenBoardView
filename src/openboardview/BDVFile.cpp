#include "BDVFile.h"

#include "utf8/utf8.h"
#include <assert.h>
#include <ctype.h>
#include <locale.h>
#include <stdint.h>
#include <string.h>

void decode_bdv(char *buf, size_t buffer_size) {
	int count = 0xa0; // First key
	for (size_t i = 0; i < buffer_size; i++) {
		if (buf[i] == '\r' && buf[i + 1] == '\n') count++; // Increment key on each new line
		char x                                 = buf[i];
		if (!(x == '\r' || x == '\n' || !x)) x = count - x;
		if (count > 285) count                 = 159;
		buf[i]                                 = x;
	}
}

bool BDVFile::verifyFormat(const char *buf, size_t buffer_size) {
	std::string sbuf(buf, buffer_size);
	if (sbuf.find("dd:1.3?,r?-=bb") != std::string::npos) return true; // encoded "<<format.asc>>"
	if ((sbuf.find("<<format.asc>>") != std::string::npos) && (sbuf.find("<<pins.asc>>") != std::string::npos)) return true;
	return false;
}

BDVFile::BDVFile(const char *buf, size_t buffer_size) {
	char *saved_locale;
	saved_locale = setlocale(LC_NUMERIC, "C"); // Use '.' as delimiter for strtod

//	memset(this, 0, sizeof(*this));
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
	memset(file_buf, 0, (3 * (1 + buffer_size)));
	memcpy(file_buf, buf, buffer_size);
	file_buf[buffer_size] = 0;
	// This is for fixing degenerate utf8
	char *arena     = &file_buf[buffer_size + 1];
	char *arena_end = file_buf + file_buf_size - 1;
	*arena_end      = 0;

	decode_bdv(file_buf, buffer_size);

	int current_block = 0;

	char **lines = stringfile(file_buf);
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
		if (!strcmp(line, "<<format.asc>>")) {
			current_block = 1;
			lines += 8; // Skip 8 unused lines before 1st point. Might not work with
			            // all files.
			continue;
		}
		if (!strcmp(line, "<<pins.asc>>")) {
			current_block = 2;
			lines += 8; // Skip 8 unused lines before 1st part
			continue;
		}
		if (!strcmp(line, "<<nails.asc>>")) {
			current_block = 3;
			lines += 7; // Skip 7 unused lines before 1st nail
			continue;
		}

		char *p = line;
		char *s;

		switch (current_block) {
			case 1: { // Format
				BRDPoint point;
				double x;
				LOAD_DOUBLE(x);
				point.x = x * 1000.0f; // OBV uses integers
				double y;
				LOAD_DOUBLE(y);
				point.y = y * 1000.0f;
				format.push_back(point);
			} break;
			case 2: { // Parts & Pins
				if (!strncmp(line, "Part", 4)) {
					p += 4; // Skip "Part" string
					BRDPart part;
					LOAD_STR(part.name);
					char *loc;
					LOAD_STR(loc);
					if (!strcmp(loc, "(T)"))
						part.type = 10; // SMD part on top
					else
						part.type    = 5; // SMD part on bottom
					part.end_of_pins = 0;
					parts.push_back(part);
				} else {
					BRDPin pin;
					pin.part = parts.size();
					int id;
					LOAD_INT(id);
					char *name;
					LOAD_STR(name);
					double posx;
					LOAD_DOUBLE(posx);
					pin.pos.x = posx * 1000.0f;
					//		pin.pos.x = posx;
					double posy;
					LOAD_DOUBLE(posy);
					pin.pos.y = posy * 1000.0f;
					//		pin.pos.y = posy;
					int layer;
					LOAD_INT(layer);
					LOAD_STR(pin.net);
					LOAD_INT(pin.probe);
					pins.push_back(pin);
					parts.back().end_of_pins = pins.size();
				}
			} break;
			case 3: { // Nails
				BRDNail nail;
				p++; // Skip first char
				LOAD_INT(nail.probe);
				double posx;
				LOAD_DOUBLE(posx);
				nail.pos.x = posx * 1000.0f;
				double posy;
				LOAD_DOUBLE(posy);
				nail.pos.y = posy * 1000.0f;
				int type;
				LOAD_INT(type);
				char *grid;
				LOAD_STR(grid);
				char *loc;
				LOAD_STR(loc);
				if (!strcmp(loc, "(T)"))
					nail.side = 1;
				else
					nail.side = 2;
				char *netid;
				LOAD_STR(netid);
				LOAD_STR(nail.net);
				nails.push_back(nail);
			} break;
		}
	}

	num_parts  = parts.size();
	num_pins   = pins.size();
	num_format = format.size();
	num_nails  = nails.size();

	setlocale(LC_NUMERIC, saved_locale); // Restore locale

	valid = current_block != 0;
fail_lines:
	free(lines_begin);
fail:;
#undef FAIL_LABEL
#undef ENSURE
}
