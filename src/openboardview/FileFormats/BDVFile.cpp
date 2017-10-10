#include "BDVFile.h"

#include "utils.h"
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

bool BDVFile::verifyFormat(std::vector<char> &buf) {
	return find_str_in_buf("dd:1.3?,r?-=bb", buf) ||
	       (find_str_in_buf("<<format.asc>>", buf) && find_str_in_buf("<<pins.asc>>", buf));
}

BDVFile::BDVFile(std::vector<char> &buf) {
	auto buffer_size = buf.size();

	char *saved_locale;
	saved_locale = setlocale(LC_NUMERIC, "C"); // Use '.' as delimiter for strtod

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

	decode_bdv(file_buf, buffer_size);

	int current_block = 0;

	char **lines = stringfile(file_buf);
	ENSURE(lines);

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

				double x = READ_DOUBLE();
				point.x  = x * 1000.0f; // OBV uses integers
				double y = READ_DOUBLE();
				point.y  = y * 1000.0f;
				format.push_back(point);
			} break;
			case 2: { // Parts & Pins
				if (!strncmp(line, "Part", 4)) {
					p += 4; // Skip "Part" string
					BRDPart part;

					part.name      = READ_STR();
					part.part_type = BRDPartType::SMD;
					char *loc      = READ_STR();
					if (!strcmp(loc, "(T)"))
						part.mounting_side = BRDPartMountingSide::Top; // SMD part on top
					else
						part.mounting_side = BRDPartMountingSide::Bottom; // SMD part on bottom
					part.end_of_pins       = 0;
					parts.push_back(part);
				} else {
					BRDPin pin;

					pin.part = parts.size();
					/*int id =*/READ_INT(); // uint
					/*char *name =*/READ_STR();
					double posx = READ_DOUBLE();
					pin.pos.x   = posx * 1000.0f;
					double posy = READ_DOUBLE();
					pin.pos.y   = posy * 1000.0f;
					/*int layer =*/READ_INT(); // uint
					pin.net   = READ_STR();
					pin.probe = READ_UINT();
					pins.push_back(pin);
					parts.back().end_of_pins = pins.size();
				}
			} break;
			case 3: { // Nails
				BRDNail nail;
				p++; // Skip first char

				nail.probe  = READ_UINT();
				double posx = READ_DOUBLE();
				nail.pos.x  = posx * 1000.0f;
				double posy = READ_DOUBLE();
				nail.pos.y  = posy * 1000.0f;
				/*int type =*/READ_INT(); // uint
				/*char *grid =*/READ_STR();
				char *loc = READ_STR();
				if (!strcmp(loc, "(T)"))
					nail.side = 1;
				else
					nail.side = 2;
				/*char *netid =*/READ_STR(); // uint
				nail.net = READ_STR();
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
}
