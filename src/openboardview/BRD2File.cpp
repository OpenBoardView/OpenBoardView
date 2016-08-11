#include "BRD2File.h"

#include <assert.h>
#include <ctype.h>
#include <iostream>
#include <stdint.h>
#include <string.h>
#include <unordered_map>

bool BRD2File::verifyFormat(const char *buf, size_t buffer_size) {
	std::string sbuf(buf, buffer_size);
	if ((sbuf.find("BRDOUT:") != std::string::npos) && (sbuf.find("NETS:") != std::string::npos)) return true;
	return false;
}

BRD2File::BRD2File(const char *buf, size_t buffer_size) {
	memset(this, 0, sizeof(*this));
	std::unordered_map<int, char *> nets; // Map between net id and net name
	char **lines_begin = nullptr;
	int num_nets       = 0;
	BRDPoint max{0, 0}; // Top-right board boundary

#define ENSURE(X)                               \
	assert(X);                                  \
	/*	if (!(X)) {                              \
	        if (lines_begin) free(lines_begin); \
	        assert(X);                          \
	        return;                             \
	    }*/

	ENSURE(buffer_size > 4);
	size_t file_buf_size = 3 * (1 + buffer_size);
	file_buf             = (char *)malloc(file_buf_size);
	memset(file_buf, 0, 3 * (1 + buffer_size));
	memcpy(file_buf, buf, buffer_size);
	file_buf[buffer_size] = 0;
	// This is for fixing degenerate utf8
	char *arena     = &file_buf[buffer_size + 1];
	char *arena_end = file_buf + file_buf_size - 1;
	*arena_end      = 0;

	int current_block = 0;

	char **lines = stringfile(file_buf);
	ENSURE(lines)
	lines_begin = lines;

	while (*lines) {
		char *line = *lines;
		++lines;

		while (isspace((uint8_t)*line)) line++;
		if (!line[0]) continue;

		char *p = line;
		char *s;

		if (strstr(line, "BRDOUT:") == line) {
			current_block = 1;
			p += 7; // Skip "BRDOUT:"
			LOAD_INT(num_format);
			LOAD_INT(max.x);
			LOAD_INT(max.y);
			ENSURE(num_format >= 0);
			ENSURE(max.x > 0);
			ENSURE(max.y > 0);
			continue;
		}
		if (strstr(line, "NETS:") == line) {
			current_block = 2;
			p += 5; // Skip "NETS:"
			LOAD_INT(num_nets);
			ENSURE(num_nets >= 0);
			continue;
		}
		if (strstr(line, "PARTS:") == line) {
			current_block = 3;
			p += 6; // Skip "PARTS:"
			LOAD_INT(num_parts);
			ENSURE(num_parts >= 0);
			continue;
		}
		if (strstr(line, "PINS:") == line) {
			current_block = 4;
			p += 5; // Skip "PINS:"
			LOAD_INT(num_pins);
			ENSURE(num_pins >= 0);
			continue;
		}
		if (strstr(line, "NAILS:") == line) {
			current_block = 5;
			p += 6; // Skip "NAILS:"
			LOAD_INT(num_nails);
			ENSURE(num_nails >= 0);
			continue;
		}

		switch (current_block) {
			case 1: { // Format
				ENSURE(format.size() < num_format);
				BRDPoint point;
				LOAD_INT(point.x);
				LOAD_INT(point.y);
				ENSURE(point.x <= max.x);
				ENSURE(point.y <= max.y);
				format.push_back(point);
			} break;

			case 2: { // Nets
				ENSURE(nets.size() < num_nets);
				int id;
				LOAD_INT(id);
				LOAD_STR(nets[id]);
			} break;

			case 3: { // PARTS
				ENSURE(parts.size() < num_parts);
				BRDPart part;
				int side;

				LOAD_STR(part.name);
				LOAD_INT(part.p1.x);
				LOAD_INT(part.p1.y);
				LOAD_INT(part.p2.x);
				LOAD_INT(part.p2.y);
				LOAD_INT(part.end_of_pins); // Warning: not end but beginning in this format
				LOAD_INT(side);
				if (side == 1)
					part.type = 10; // SMD part on top
				else if (side == 2)
					part.type = 5; // SMD part on bottom

				parts.push_back(part);
			} break;

			case 4: { // PINS
				ENSURE(pins.size() < num_pins);
				BRDPin pin;
				int netid, side;

				LOAD_INT(pin.pos.x);
				LOAD_INT(pin.pos.y);
				LOAD_INT(netid);
				LOAD_INT(pin.side);

				try {
					pin.net = nets.at(netid);
				} catch (const std::out_of_range &e) {
					pin.net = "";
				}

				pin.probe = 1;
				pin.part  = 0;
				pins.push_back(pin);
			} break;

			case 5: { // NAILS
				ENSURE(nails.size() < num_nails);
				BRDNail nail;
				int netid;
				LOAD_INT(nail.probe);
				LOAD_INT(nail.pos.x);
				LOAD_INT(nail.pos.y);
				LOAD_INT(netid);
				nail.net = nets.at(netid);
				LOAD_INT(nail.side);
				nails.push_back(nail);

			} break;
			default: continue;
		}
	}

	ENSURE(num_format == format.size());
	ENSURE(num_nets == nets.size());
	ENSURE(num_parts == parts.size());
	ENSURE(num_pins == pins.size());
	ENSURE(num_nails == nails.size());

	/*
	 * Postprocess the data.  Specifically we need to allocate
	 * pins to parts.  So with this it's easiest to just iterate
	 * through the parts and pick up the pins required
	 */
	{
		int pei;     // pin end index (part[i+1].pin# -1
		int cpi = 0; // current pin index
		for (decltype(parts)::size_type i = 0; i < parts.size(); i++) {
			bool isDIP = true;

			if (parts[i].type == 5) { // Part on bottom
				parts[i].p1.y = max.y - parts[i].p1.y;
				parts[i].p2.y = max.y - parts[i].p2.y;
			}

			if (i == parts.size() - 1) {
				pei = pins.size();
			} else {
				pei = parts[i + 1].end_of_pins; // Again, not end of pins but beginning
			}

			while (cpi < pei) {
				pins[cpi].part                           = i + 1;
				if (pins[cpi].side != 1) pins[cpi].pos.y = max.y - pins[cpi].pos.y;
				if ((pins[cpi].side == 1 && parts[i].type == 10) ||
				    (pins[cpi].side == 2 && parts[i].type == 5)) // Pins on the same side as the part
					isDIP = false;
				cpi++;
			}

			if (isDIP) parts[i].type /= 5; // All pins are through hole so part is DIP
		}
	}

	for (auto i = 1; i <= 2; i++) { // Add dummy parts for probe points on both sides
		BRDPart part;
		part.name        = "...";
		part.type        = 5 * i; // First part is bottom, last is top.
		part.end_of_pins = 0;     // Unused
		parts.push_back(part);
	}

	for (auto &nail : nails) {
		BRDPin pin;
		pin.pos = nail.pos;
		if (nail.side == 1) {
			pin.part = parts.size();
		} else {
			pin.pos.y = max.y - pin.pos.y;
			pin.part  = parts.size() - 1;
		}
		pin.probe = nail.probe;
		pin.side  = nail.side;
		pin.net   = nail.net;
		pins.push_back(pin);
	}

	valid = current_block != 0;
#undef ENSURE
}
