#include "CSTFile.h"
#include "utils.h"

#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <unordered_map>

/*
 * Creates fake points for outline using outermost pins plus some margin.
 */
#define OUTLINE_MARGIN 20
void CSTFile::gen_outline() {
	// Determine board outline
	int minx =
	    std::min_element(pins.begin(), pins.end(), [](BRDPin a, BRDPin b) { return a.pos.x < b.pos.x; })->pos.x - OUTLINE_MARGIN;
	int maxx =
	    std::max_element(pins.begin(), pins.end(), [](BRDPin a, BRDPin b) { return a.pos.x < b.pos.x; })->pos.x + OUTLINE_MARGIN;
	int miny =
	    std::min_element(pins.begin(), pins.end(), [](BRDPin a, BRDPin b) { return a.pos.y < b.pos.y; })->pos.y - OUTLINE_MARGIN;
	int maxy =
	    std::max_element(pins.begin(), pins.end(), [](BRDPin a, BRDPin b) { return a.pos.y < b.pos.y; })->pos.y + OUTLINE_MARGIN;
	format.push_back({minx, miny});
	format.push_back({maxx, miny});
	format.push_back({maxx, maxy});
	format.push_back({minx, maxy});
	format.push_back({minx, miny});
}
#undef OUTLINE_MARGIN

/*
 * Updates element counts
 */
void CSTFile::update_counts() {
	num_parts  = parts.size();
	num_pins   = pins.size();
	num_format = format.size();
	num_nails  = nails.size();
}

short read_short(char *&p) {
	short s = *(reinterpret_cast<short *>(p));
	p += 2;
	return s;
}

short read_byte(char *&p) {
	return *p++;
}

char *read_cstring(char *&p, int size) {
	char *str = p;
	p += size;
	return str;
}

std::string read_string(char *&p, int size) {
	std::string s(p, size);
	p += size;
	return s;
}

CSTFile::CSTFile(std::vector<char> &buf) {
	auto buffer_size = buf.size();

	ENSURE(buffer_size > 4);
	size_t file_buf_size = 3 * (1 + buffer_size);
	file_buf             = (char *)calloc(1, file_buf_size);
	ENSURE(file_buf != nullptr);

	std::copy(buf.begin(), buf.end(), file_buf);
	file_buf[buffer_size] = 0;


	short string_length;
	char *p = file_buf; // Not quite C++ but it's easier to work with a raw pointer here

	num_parts = read_short(p);
	p += 4;                        // New section signature
	string_length = read_short(p); // Section name length
	read_string(p, string_length); // Section name

	for (unsigned int i = 0; i < num_parts; i++) {
		string_length       = read_byte(p);                   // Part name length
		char *name          = read_cstring(p, string_length); // Part name
		name[string_length] = '\0'; // Convert to C-string. Warning: overwrites the data byte just after the name in file_buf. If
		                            // it's needed, copy it before this line.
		p += 4;                     // unknown
		int layer = read_byte(p);

		BRDPart part;
		part.name      = name;
		part.part_type = BRDPartType::SMD;
		if (layer == 0x0C) {
			// FIXME: Layer 12
			part.mounting_side = BRDPartMountingSide::Top;
		} else if (layer == 0x01) {
			// Layer 1
			part.mounting_side = BRDPartMountingSide::Bottom;
		}

		part.end_of_pins = 0;
		parts.push_back(part);

		p += 6; // unknown
	}

	p -= 2; // begining of nets list
	num_nets = read_short(p);
	for (unsigned int i = 0; i < num_nets; i++) {
		string_length = read_byte(p);                   // Net name length
		p[-1]         = '\0';                           // Convert previous name to C-string
		char *name    = read_cstring(p, string_length); // Net name
		nets.push_back(name);
	}
	p[0] = '\0'; // Convert last name to C-string

	// Add dummy parts for orphan pins on both sides
	BRDPart part;
	part.name          = "...";
	part.mounting_side = BRDPartMountingSide::Both; // FIXME: Both sides?
	part.part_type     = BRDPartType::ThroughHole;
	part.end_of_pins   = 0; // Unused
	parts.push_back(part);

	while (strncmp(p, "CPad", 4)) p++; // Search for the CPad section
	p -= 8;                            // Go back to begining of section header

	num_pins = read_short(p);
	p += 10; // Skip past section header

	for (unsigned int i = 0; i < num_pins; i++) {
		BRDPin pin;

		short part_id = read_short(p); // dev_id
		if (part_id >= 0)
			pin.part = part_id + 1;
		else
			pin.part = parts.size(); // No associated part, use dummy

		pin.probe    = read_short(p); // probe? not sure
		short net_id = read_short(p); // net_id

		pin.net   = nets.at(net_id);
		pin.pos.x = read_short(p);
		pin.pos.y = read_short(p);

		short shape = read_short(p); // Might correspond to a shape in the CShape section

		pins.push_back(pin);

		p += 4; // seems unused
	}

	gen_outline(); // We haven't figured out how to get the outline yet

	update_counts(); // FIXME: useless?

	valid = true; // FIXME: better way to ensure that?
}
