#include "BVR3File.h"

#include "utils.h"
#include <cmath>
#include <cctype>
#include <clocale>
#include <cstdint>
#include <cstring>

bool BVR3File::verifyFormat(std::vector<char> &buf) {
	return find_str_in_buf("BVRAW_FORMAT_3", buf);
}

BVR3File::BVR3File(std::vector<char> &buf) {
	auto buffer_size = buf.size();

	char *saved_locale = setlocale(LC_NUMERIC, "C"); // Use '.' as delimiter for strtod

	ENSURE_OR_FAIL(buffer_size > 4, error_msg, return);
	size_t file_buf_size = 3 * (1 + buffer_size);
	file_buf             = (char *)calloc(1, file_buf_size);
	ENSURE_OR_FAIL(file_buf != nullptr, error_msg, return);

	std::copy(buf.begin(), buf.end(), file_buf);
	file_buf[buffer_size] = 0;
	// This is for fixing degenerate utf8
	char *arena     = &file_buf[buffer_size + 1];
	char *arena_end = file_buf + file_buf_size - 1;
	*arena_end      = 0;

	int current_block = 0;
	BRDPart empty_part;
	BRDPin empty_pin;
	BRDPart part;
	BRDPin pin;

	std::vector<char *> lines;
	stringfile(file_buf, lines);

	fprintf(stderr, "START\n");

	for (char *line : lines) {
		while (isspace((uint8_t)*line)) line++;
		if (!line[0]) continue;

		char *p = line;
		char *s;

		if (!strncmp(line, "PART_NAME ", 10)) {
			p += 10;
			part.name = READ_STR();
			continue;
		}

		if (!strncmp(line, "PART_SIDE ", 10)) {
			p += 10;
			char *side = READ_STR();
			if (!strcmp(side, "T"))
				part.mounting_side = BRDPartMountingSide::Top;
			else
				part.mounting_side = BRDPartMountingSide::Bottom;
			continue;
		}

		if (!strncmp(line, "PART_ORIGIN ", 12)) {
			p += 12;
			double origin_x = READ_DOUBLE();
			part.p1.x = trunc(origin_x);
			double origin_y = READ_DOUBLE();
			part.p1.y = trunc(origin_y);
			continue;
		}

		if (!strncmp(line, "PART_MOUNT ", 11)) {
			p += 11;
			char *mount = READ_STR();
			if (!strcmp(mount, "SMD"))
				part.part_type = BRDPartType::SMD;
			else
				part.part_type = BRDPartType::ThroughHole;
			continue;
		}

		if (!strncmp(line, "PIN_ID ", 7)) {
			p += 7;
			pin.snum = READ_STR();
			pin.part = parts.size() + 1; // Pin is for current part, but will not yet have been added to parts vector
			continue;
		}

		if (!strncmp(line, "PIN_NUMBER ", 11)) {
			// Ignored, not sure if relevant for BRDPin
			continue;
		}

		if (!strncmp(line, "PIN_NAME ", 9)) {
			p += 9;
			pin.name = READ_STR();
			continue;
		}

		if (!strncmp(line, "PIN_SIDE ", 9)) {
			// Ignored, not sure if relevant for BRDPin
			continue;
		}

		if (!strncmp(line, "PIN_ORIGIN ", 11)) {
			p += 11;
			double origin_x = READ_DOUBLE();
			pin.pos.x = trunc(origin_x);
			double origin_y = READ_DOUBLE();
			pin.pos.y = trunc(origin_y);
			continue;
		}

		if (!strncmp(line, "PIN_RADIUS ", 11)) {
			p += 11;
			pin.radius = READ_DOUBLE();
			continue;
		}

		if (!strncmp(line, "PIN_NET ", 8)) {
			p += 8;
			pin.net = READ_STR();
			continue;
		}

		if (!strncmp(line, "PIN_TYPE ", 9)) {
			// Ignored, not sure if relevant for BRDPin
			continue;
		}

		if (!strncmp(line, "PIN_COMMENT ", 12)) {
			// Ignored, not sure if relevant for BRDPin
			continue;
		}

		if (!strncmp(line, "PIN_OUTLINE_RELATIVE ", 21)) {
			// Ignored, not sure if relevant for BRDPin
			continue;
		}

		if (!strcmp(line, "PIN_END")) {
			pins.push_back(pin);
			fprintf(stderr, "BRDPin: {\n");
			fprintf(stderr, "  snum: '%s'\n", pin.snum);
			fprintf(stderr, "  name: '%s'\n", pin.name);
			fprintf(stderr, "  pos: (%d, %d)\n", pin.pos.x, pin.pos.y);
			fprintf(stderr, "  radius: %f\n", pin.radius);
			fprintf(stderr, "  net: '%s'\n", pin.net);
			fprintf(stderr, "  part: %u\n", pin.part);
			fprintf(stderr, "  side: %u\n", pin.side);
			fprintf(stderr, "}\n");
			pin = empty_pin;
			continue;
		}

		if (!strcmp(line, "PART_END")) {
			part.end_of_pins = pins.size();
			parts.push_back(part);
			fprintf(stderr, "BRDPart: {\n");
			fprintf(stderr, "  name: '%s'\n", part.name);
			fprintf(stderr, "  p1: (%d, %d)\n", part.p1.x, part.p1.y);
			fprintf(stderr, "  p2: (%d, %d)\n", part.p2.x, part.p2.y);
			fprintf(stderr, "  mounting_side: %d\n", part.mounting_side);
			fprintf(stderr, "  part_type: %d\n", part.part_type);
			fprintf(stderr, "  end_of_pins: %u\n", part.end_of_pins);
			fprintf(stderr, "}\n");
			part = empty_part;
			continue;
		}

		if (!strncmp(line, "OUTLINE_SEGMENTED ", 18)) {
			p += 18;
			while (p[0]) {
				BRDPoint point;
				double x = READ_DOUBLE();
				point.x  = trunc(x);
				double y = READ_DOUBLE();
				point.y  = trunc(y);
				//auto existing_point = std::find(format.begin(), format.end(), point);
				//if (existing_point != format.end())
				//	format.erase(existing_point);
				//if (std::find(format.begin(), format.end(), point) == format.end())
				format.push_back(point);
			}

			fprintf(stderr, "format:\n");
			for (BRDPoint format_point : format)
				fprintf(stderr, "(%d, %d)\n", format_point.x, format_point.y);
			continue;
		}
	}

	num_parts  = parts.size();
	num_pins   = pins.size();
	num_format = format.size();
	num_nails  = nails.size();

	setlocale(LC_NUMERIC, saved_locale); // Restore locale

	valid = true; // TODO
}
