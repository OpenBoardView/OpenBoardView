#include "BRDFile.h"

#include "utils.h"
#include <cctype>
#include <stdexcept>
#include <cstdint>
#include <cstring>
#include <unordered_map>

// Header for recognizing a BRD file
decltype(BRDFile::signature) constexpr BRDFile::signature;

/*
 * Returns true if the file format seems to be BRD.
 * Uses std::string::find() on a std::string rather than strstr() on the buffer because the latter expects a null-terminated string.
 */
bool BRDFile::verifyFormat(std::vector<char> &buf) {
	if (buf.size() < signature.size()) return false; // C++14 implements a safer std::equal where this is not needed
	if (std::equal(signature.begin(), signature.end(), buf.begin(), [](const uint8_t &i, const char &j) {
		    return i == reinterpret_cast<const uint8_t &>(j);
		}))
		return true;
	return find_str_in_buf("str_length:", buf) && find_str_in_buf("var_data:", buf);
}

BRDFile::BRDFile(std::vector<char> &buf) {
	auto buffer_size = buf.size();
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
	std::vector<char*> lines;
	stringfile(file_buf, lines);

	for (char *line : lines) {
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
		if (!strcmp(line, "Format:") || !strcmp(line, "format:")) {
			current_block = 3;
			continue;
		}
		if (!strcmp(line, "Parts:") || !strcmp(line, "Pins1:")) {
			current_block = 4;
			continue;
		}
		if (!strcmp(line, "Pins:") || !strcmp(line, "Pins2:")) {
			current_block = 5;
			continue;
		}
		if (!strcmp(line, "Nails:")) {
			current_block = 6;
			continue;
		}

		char *p = line;
		char *s;
		unsigned int tmp = 0;

		switch (current_block) {
			case 2: { // var_data
				num_format = READ_UINT();
				num_parts  = READ_UINT();
				num_pins   = READ_UINT();
				num_nails  = READ_UINT();
			} break;
			case 3: { // Format
				ENSURE(format.size() < num_format, error_msg);
				BRDPoint fmt;
				fmt.x = strtol(p, &p, 10);
				fmt.y = strtol(p, &p, 10);
				format.push_back(fmt);
			} break;
			case 4: { // Parts
				ENSURE(parts.size() < num_parts, error_msg);
				BRDPart part;
				part.name      = READ_STR();
				tmp            = READ_UINT(); // Type and layer, actually.
				part.part_type = (tmp & 0xc) ? BRDPartType::SMD : BRDPartType::ThroughHole;
				if (tmp == 1 || (4 <= tmp && tmp < 8)) part.mounting_side = BRDPartMountingSide::Top;
				if (tmp == 2 || (8 <= tmp)) part.mounting_side            = BRDPartMountingSide::Bottom;
				part.end_of_pins                                          = READ_UINT();
				ENSURE(part.end_of_pins <= num_pins, error_msg);
				parts.push_back(part);
			} break;
			case 5: { // Pins
				ENSURE(pins.size() < num_pins, error_msg);
				BRDPin pin;
				pin.pos.x = READ_INT();
				pin.pos.y = READ_INT();
				pin.probe = READ_INT(); // Can be negative (-99)
				pin.part  = READ_UINT();
				ENSURE(pin.part <= num_parts, error_msg);
				pin.net = READ_STR();
				pins.push_back(pin);
			} break;
			case 6: { // Nails
				ENSURE(nails.size() < num_nails, error_msg);
				BRDNail nail;
				nail.probe = READ_UINT();
				nail.pos.x = READ_INT();
				nail.pos.y = READ_INT();
				nail.side  = READ_UINT() == 1 ? BRDPartMountingSide::Top : BRDPartMountingSide::Bottom;
				nail.net   = READ_STR();
				nails.push_back(nail);
			} break;
		}
	}

	// Lenovo brd variant, find net from nail
	std::unordered_map<int, const char *> nailsToNets; // Map between net id and net name
	for (auto &nail : nails) {
		nailsToNets[nail.probe] = nail.net;
	}

	for (auto &pin : pins) {
		if (!strcmp(pin.net, "")) {
			try {
				pin.net = nailsToNets.at(pin.probe);
			} catch (const std::out_of_range &e) {
				pin.net = "";
			}
		}
		switch (parts[pin.part - 1].mounting_side) {
			case BRDPartMountingSide::Top:    pin.side = BRDPinSide::Top;    break;
			case BRDPartMountingSide::Bottom: pin.side = BRDPinSide::Bottom; break;
			case BRDPartMountingSide::Both:   pin.side = BRDPinSide::Both;   break;
		}
	}

	valid = current_block != 0;
}
