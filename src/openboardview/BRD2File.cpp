#include "BRD2File.h"

#include "utf8/utf8.h"
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <unordered_map>

#include <iostream>

bool BRD2File::verifyFormat(const char *buf, size_t buffer_size) {
	std::string sbuf(buf, buffer_size);
	if ((sbuf.find("BRDOUT:") != std::string::npos) && (sbuf.find("NETS:") != std::string::npos)) return true;
	return false;
}

BRD2File::BRD2File(const char *buf, size_t buffer_size) {
	memset(this, 0, sizeof(*this));
	std::unordered_map<int, char *> nets;
	char **lines_begin = nullptr;
	int pins_idx       = 0;
	int num_nets       = 0;
	int max_y          = 0;
	int max_x          = 0;

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

		//		fprintf(stderr,"Parsing:%s\n",line);

		while (isspace((uint8_t)*line)) line++;
		if (!line[0]) continue;

		char *p = line;
		char *s;

		if (strstr(line, "BRDOUT:") == line) {
			current_block = 1;
			p += 7; // Skip "BRDOUT:"
			LOAD_INT(num_format);
			ENSURE(num_format >= 0);
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
				format.push_back(point);

				if (point.x > max_x) max_x = point.x;
				if (point.y > max_y) max_y = point.y;

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
				LOAD_INT(part.x);
				LOAD_INT(part.y);
				LOAD_INT(part.z);
				LOAD_INT(part.t);
				LOAD_INT(part.end_of_pins);
				LOAD_INT(side);
				if (side == 1)
					part.type = 10; // SMD part on top
				else if (side == 2)
					part.type = 5; // SMD part on bottom

				parts.push_back(part);
			} break;

			case 4: { // PINS
				pins_idx++;
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
					// pin.net = nullptr;
					//	"...";
				}
				pin.probe = 1;
				pin.part  = 0;
#ifdef PIERNOV
				for (decltype(parts)::size_type i = 0; i < parts.size(); i++) { // Yep, inefficient but I've got no better idea
					if (pin.pos.x >= parts[i].x && pin.pos.x <= parts[i].z && pin.pos.y >= parts[i].y &&
					    pin.pos.y <= parts[i].t /*&& parts[i].end_of_pins >= pins_idx*/) {
						if ((side == 1 && parts[i].type == 10) || (side == 2 && parts[i].type == 5)) {
							pin.part = i + 1;
							break;
						}
					}
				}
				//				ENSURE(pin.part > 0);
				if (pin.part <= 0) {
					std::cout << "No part for pin. Net: " << (pin.net ? pin.net : "...") << " x: " << pin.pos.x
					          << " y: " << pin.pos.y << std::endl;
					break;
				}
#endif
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
	//	ENSURE(num_nets == nets.size());
	ENSURE(num_parts == parts.size());
	//	ENSURE(num_pins == pins.size());
	ENSURE(num_nails == nails.size());

	/*
	 * Postprocess the data.  Specifically we need to allocate
	 * pins to parts.  So with this it's easiest to just iterate
	 * through the parts and pick up the pins required
	 */
	{
		int pei;                                                        // pin end index (part[i+1].pin# -1
		int cpi = 0;                                                    // current pin index
		for (decltype(parts)::size_type i = 0; i < parts.size(); i++) { // Yep, inefficient but I've got no better idea

			if (i == parts.size() - 1) {
				pei = pins.size();
			} else {
				pei = parts[i + 1].end_of_pins;
			}

			while (cpi < pei) {
				pins[cpi].part                           = i + 1;
				if (pins[cpi].side != 1) pins[cpi].pos.y = max_y - pins[cpi].pos.y;
				cpi++;
			}
		}
	}

	num_parts  = parts.size();
	num_pins   = pins.size();
	num_format = format.size();
	num_nails  = nails.size();

	valid = current_block != 0;
#undef ENSURE
}
