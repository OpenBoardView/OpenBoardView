#include "BRD2File.h"

#include "utils.h"
#include <cctype>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <unordered_map>

bool BRD2File::verifyFormat(std::vector<char> &buf) {
	return find_str_in_buf("BRDOUT:", buf) && find_str_in_buf("NETS:", buf);
}

BRD2File::BRD2File(std::vector<char> &buf) {
	auto buffer_size = buf.size();
	std::unordered_map<int, char *> nets; // Map between net id and net name
	unsigned int num_nets = 0;
	BRDPoint max{0, 0}; // Top-right board boundary

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

	std::vector<char *>  lines;
	stringfile(file_buf, lines);

	for (char *line : lines) {
		while (isspace((uint8_t)*line)) line++;
		if (!line[0]) continue;

		char *p = line;
		char *s;

		if (strstr(line, "BRDOUT:") == line) {
			current_block = 1;
			p += 7; // Skip "BRDOUT:"
			num_format = READ_UINT();
			max.x      = READ_INT();
			max.y      = READ_INT();
			continue;
		}
		if (strstr(line, "NETS:") == line) {
			current_block = 2;
			p += 5; // Skip "NETS:"
			num_nets = READ_UINT();
			continue;
		}
		if (strstr(line, "PARTS:") == line) {
			current_block = 3;
			p += 6; // Skip "PARTS:"
			num_parts = READ_UINT();
			continue;
		}
		if (strstr(line, "PINS:") == line) {
			current_block = 4;
			p += 5; // Skip "PINS:"
			num_pins = READ_UINT();
			continue;
		}
		if (strstr(line, "NAILS:") == line) {
			current_block = 5;
			p += 6; // Skip "NAILS:"
			num_nails = READ_UINT();
			continue;
		}

		switch (current_block) {
			case 1: { // Format
				ENSURE(format.size() < num_format, error_msg);
				BRDPoint point;
				point.x = READ_INT();
				point.y = READ_INT();
				ENSURE(point.x <= max.x, error_msg);
				ENSURE(point.y <= max.y, error_msg);
				format.push_back(point);
			} break;

			case 2: { // Nets
				ENSURE(nets.size() < num_nets, error_msg);
				int id   = READ_UINT();
				nets[id] = READ_STR();
			} break;

			case 3: { // PARTS
				ENSURE(parts.size() < num_parts, error_msg);
				BRDPart part;

				part.name        = READ_STR();
				part.p1.x        = READ_INT();
				part.p1.y        = READ_INT();
				part.p2.x        = READ_INT();
				part.p2.y        = READ_INT();
				part.end_of_pins = READ_UINT(); // Warning: not end but beginning in this format
				part.part_type   = BRDPartType::SMD;
				int side         = READ_UINT();
				if (side == 1)
					part.mounting_side = BRDPartMountingSide::Top; // SMD part on top
				else if (side == 2)
					part.mounting_side = BRDPartMountingSide::Bottom; // SMD part on bottom
				else //0
					part.mounting_side = BRDPartMountingSide::Both;

				parts.push_back(part);
			} break;

			case 4: { // PINS
				ENSURE(pins.size() < num_pins, error_msg);
				BRDPin pin;

				pin.pos.x = READ_INT();
				pin.pos.y = READ_INT();
				int netid = READ_UINT();
				unsigned int side  = READ_UINT();
				if (side == 1)
					pin.side = BRDPinSide::Top;
				else if (side == 2)
					pin.side = BRDPinSide::Bottom;
				else //0
					pin.side = BRDPinSide::Both;

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
				ENSURE(nails.size() < num_nails, error_msg);
				BRDNail nail;

				nail.probe = READ_UINT();
				nail.pos.x = READ_INT();
				nail.pos.y = READ_INT();
				int netid  = READ_UINT();

				auto inet = nets.find(netid);
				if (inet != nets.end())
					nail.net = inet->second;
				else {
					nail.net = "UNCONNECTED";
					std::cerr << "Missing net id: " << netid << std::endl;
				}

				bool nail_is_top = READ_UINT() == 1;
				if (nail_is_top) {
					nail.side = BRDPartMountingSide::Top;
				} else {
					nail.side = BRDPartMountingSide::Bottom;
					nail.pos.y = max.y - nail.pos.y;
				}
				nails.push_back(nail);

			} break;
			default: continue;
		}
	}

	ENSURE(num_format == format.size(), error_msg);
	ENSURE(num_nets == nets.size(), error_msg);
	ENSURE(num_parts == parts.size(), error_msg);
	ENSURE(num_pins == pins.size(), error_msg);
	ENSURE(num_nails == nails.size(), error_msg);

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

			if (parts[i].mounting_side == BRDPartMountingSide::Bottom) { // Part on bottom
				parts[i].p1.y = max.y - parts[i].p1.y;
				parts[i].p2.y = max.y - parts[i].p2.y;
			}

			if (i == parts.size() - 1) {
				pei = pins.size();
			} else {
				pei = parts[i + 1].end_of_pins; // Again, not end of pins but beginning
			}

			auto firstPinSide = pins[cpi].side;
			while (cpi < pei) {
				pins[cpi].part                           = i + 1;
				if (pins[cpi].side != BRDPinSide::Top) pins[cpi].pos.y = max.y - pins[cpi].pos.y;
				if ((pins[cpi].side == BRDPinSide::Top && parts[i].mounting_side == BRDPartMountingSide::Top) ||
				    (pins[cpi].side == BRDPinSide::Bottom && parts[i].mounting_side == BRDPartMountingSide::Bottom)) {
					// Pins on the same side as the part
					isDIP = false;
				}
				// If pins appear on both sides for this part, mounting side of part is both even if SMD
				if (pins[cpi].side != firstPinSide) {
					parts[i].mounting_side = BRDPartMountingSide::Both;
				}
				cpi++;
			}

			if (isDIP) {
				// All pins are through hole so part is DIP
				parts[i].part_type     = BRDPartType::ThroughHole;
				parts[i].mounting_side = BRDPartMountingSide::Both;
			} else {
				parts[i].part_type = BRDPartType::SMD;
			}
		}
	}

	for (auto i = 1; i <= 2; i++) { // Add dummy parts for probe points on both sides
		BRDPart part;
		part.name = "...";
		part.mounting_side =
		    (i == 1 ? BRDPartMountingSide::Bottom : BRDPartMountingSide::Top); // First part is bottom, last is top.
		part.end_of_pins = 0;                                                  // Unused
		parts.push_back(part);
	}

	AddNailsAsPins();

	valid = current_block != 0;
}
