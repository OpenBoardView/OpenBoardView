#include "CADFile.h"

#include "utils.h"
#include <assert.h>
#include <algorithm>
#include <cmath>
#include <ctype.h>
#include <locale.h>
#include <stdint.h>
#include <string.h>
#include <unordered_map>

#define OUTLINE_MARGIN 20
void CADFile::gen_outline() {
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

bool CADFile::verifyFormat(std::vector<char> &buf) {
	return (find_str_in_buf("P_ADDP", buf) && find_str_in_buf("C_PIN", buf));
}

CADFile::CADFile(std::vector<char> &buf) {
	auto buffer_size = buf.size();
	float multiplier = 1000.0f;
	char *saved_locale;
	saved_locale  = setlocale(LC_NUMERIC, "C"); // Use '.' as delimiter for strtod

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

	int current_block = 0;
	std::unordered_map<std::string, int> parts_id; // map between part name and part number
	char *nailnet; // Net name for VIA

	char **lines = stringfile(file_buf);
	ENSURE(lines);

	while (*lines) {
		char *line = *lines;
		++lines;
		while (isspace((uint8_t)*line)) line++;
		if (!line[0]) continue;

			if (!strncmp(line, "COMP", 4)) {
				current_block = 1;
			} else if (!strncmp(line, "C_PIN", 5)) {
				current_block = 2;
			} else if (!strncmp(line, "NET ", 4)) {
				current_block = 3;
			} else if (!strncmp(line, "N_VIA", 5)) {
				current_block = 4;
			} else {
				current_block = -1;
				continue;
			}
		char *p = line;
		char *s;

		switch (current_block) {
			case 1: { // Parts
				BRDPart part;
				BRDPin pin;
				/*char *TYPE    =*/READ_STR();
				part.name       = READ_STR();
				/*char *Part(NR)=*/READ_STR();
				/*char *Unknown =*/READ_STR();
				/*char *Unknown =*/READ_STR();
				/*char *X       =*/READ_STR();
				/*char *Y       =*/READ_STR();
				char *loc       = READ_STR();
				/*int *Unknown  =*/READ_STR();
				part.part_type  = BRDPartType::SMD;
				if (!strcmp(loc, "1"))//top 1 bot 2
					part.mounting_side = BRDPartMountingSide::Top; // SMD part on top
				else
					part.mounting_side = BRDPartMountingSide::Bottom; // SMD part on bottom
				part.end_of_pins       = 0;
				parts.push_back(part);
				parts_id[part.name] = parts.size();
			} break;
            
			case 2: { // Pins
				BRDPin pin;
				/*char *TYPE  =*/READ_STR();
				char *part    = READ_STR();
				char *ptr; 
				ptr = strchr(part, '-'); //remove all after
				if (ptr != NULL)         //
					{*ptr = '\0';}       //
				pin.part       = parts_id.at(part);
				double posx    = READ_DOUBLE();
				pin.pos.x      = posx * multiplier;
				double posy    = READ_DOUBLE();
				pin.pos.y      = posy * multiplier;
				/*int *Unknown =*/READ_DOUBLE();
				/*int *Unknown =*/READ_DOUBLE();
				/*int *Unknown =*/READ_DOUBLE();
				/*int *Unknown =*/READ_STR();
				pin.net        = READ_STR();
				pins.push_back(pin);
			} break;
			case 3: {   // NET
				/*char *TYPE  =*/READ_STR();
				nailnet = READ_STR();
			} break;
			case 4: { // VIA
				BRDNail nail;
				nail.net     = nailnet;
				/*STR *TYPE  =*/READ_STR();
				double posx  = READ_DOUBLE();
				nail.pos.x   = posx * multiplier;
				double posy  = READ_DOUBLE();
				nail.pos.y   = posy * multiplier;
				/*STR        =*/READ_STR();
				nail.side    = READ_DOUBLE();
				/*double     =*/READ_DOUBLE();
				nails.push_back(nail);
			} break;
			default: continue;
		}
	}
	gen_outline();
	num_parts  = parts.size();
	num_pins   = pins.size();
	num_format = format.size();
	num_nails  = nails.size();

	setlocale(LC_NUMERIC, saved_locale); // Restore locale

	valid = current_block != 0;
}
