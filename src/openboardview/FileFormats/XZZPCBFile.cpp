#include "XZZPCBFile.h"
#include "des.h"
#include "utils.h"
#include <algorithm>
#include <cstdint>
#include <cstdio> // For fprintf
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

/*
 * Credit to @huertas for DES functions
 * Also credit to @inflex and @MuertoGB for help with cracking the encryption + decoding the format
 */


static unsigned char hexconv[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0,  0,  0,  0,  0,  10, 11, 12, 13, 14, 15, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15, 0,  0,  0,  0,  0,  0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};

void XZZPCBFile::des_decrypt(std::vector<char> &buf) {
	std::vector<uint16_t> byteList = {0xE0, 0xCF, 0x2E, 0x9F, 0x3C, 0x33, 0x3C, 0x33};

	std::ostringstream a;
	for (size_t i = 0; i < byteList.size(); i += 2) {
		uint16_t value = (byteList[i] << 8) | byteList[i + 1];
		value ^= 0x3C33; // <3
		a << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << value;
	}
	std::string b = a.str();
	char *c       = new char[b.length() + 1];
	std::copy(b.begin(), b.end(), c);
	c[b.length()] = '\0';

	std::vector<uint8_t> buf_uint8(buf.begin(), buf.end());
	uint8_t *p  = buf_uint8.data();
	uint8_t *ep = p + buf_uint8.size();

	uint64_t k     = 0x0000000000000000;
	const char *kp = c;
	for (int i = 0; i < 8; i++) {
		uint64_t v = hexconv[(int)*kp] * 16 + hexconv[(int)*(kp + 1)];
		k |= (v << ((7 - i) * 8));
		kp += 2;
	}

	std::vector<uint8_t> decrypted_buf;
	while (p < ep) {
		unsigned char e[8];
		unsigned char d[8];
		uint64_t d64;
		uint64_t e64;

		// Build encrypted block
		for (int i = 0; i < 8; i++) {
			e[7 - i] = *p;
			p++;
		}
		memcpy(&e64, e, 8);

		// Decode/decrypt
		d64 = des(e64, k, 'd');

		// Reverse d64 and append to decrypted_buf
		for (int i = 0; i < 8; i++) {
			d[i] = (d64 >> (i * 8)) & 0xff; // Extract each byte
		}
		for (int i = 7; i >= 0; i--) {
			decrypted_buf.push_back(d[i]); // Append in reverse order
		}
	}
	buf = std::vector<char>(decrypted_buf.begin(), decrypted_buf.end());
}

std::vector<std::pair<BRDPoint, BRDPoint>> XZZPCBFile::xzz_arc_to_segments(int startAngle, int endAngle, int r, BRDPoint pc) {
	const int numPoints = 10;
	std::vector<std::pair<BRDPoint, BRDPoint>> arc_segments{};

	if (startAngle > endAngle) {
		std::swap(startAngle, endAngle);
	}

	if (endAngle - startAngle > 180) {
		startAngle += 360;
	}

	double startAngleD = static_cast<double>(startAngle);
	double endAngleD   = static_cast<double>(endAngle);
	double rD          = static_cast<double>(r);
	double pc_xD       = static_cast<double>(pc.x);
	double pc_yD       = static_cast<double>(pc.y);

	const double degToRad = 3.14159265358979323846 / 180.0;
	startAngleD *= degToRad;
	endAngleD *= degToRad;

	double angleStep = (endAngleD - startAngleD) / (numPoints - 1);

	BRDPoint pold = {static_cast<int>(static_cast<float>(pc_xD + rD * cos(startAngleD))),
	                 static_cast<int>(static_cast<float>(pc_yD + rD * sin(startAngleD)))};
	for (int i = 1; i < numPoints; ++i) {
		double angle = startAngleD + i * angleStep;
		BRDPoint p   = {static_cast<int>(pc_xD + rD * cos(angle)), static_cast<int>(pc_yD + rD * sin(angle))};
		arc_segments.push_back({pold, p});
		pold = p;
	}

	return arc_segments;
}

bool XZZPCBFile::verifyFormat(std::vector<char> &buf) {
	bool raw = std::string(buf.begin(), buf.begin() + 6) == "XZZPCB";
	if (raw) {
		return true;
	}

	if (buf[0x10] != 0x00) {
		uint8_t xor_key = buf[0x10];
		std::vector<char> xor_buf(buf.begin(), buf.begin() + 6);
		for (int i = 0; i < 6; ++i) {
			xor_buf[i] ^= xor_key;
		}
		return std::string(xor_buf.begin(), xor_buf.end()) == "XZZPCB";
	}

	return false;
}

XZZPCBFile::XZZPCBFile(std::vector<char> &buf, std::string filepath) {
	std::list<std::pair<BRDPoint, BRDPoint>> outline_segments;

	auto v6v6555v6v6       = std::vector<uint8_t>{0x76, 0x36, 0x76, 0x36, 0x35, 0x35, 0x35, 0x76, 0x36, 0x76, 0x36};
	auto v6v6555v6v6_found = std::search(buf.begin(), buf.end(), v6v6555v6v6.begin(), v6v6555v6v6.end());

	if (v6v6555v6v6_found != buf.end()) {
		if (buf[0x10] != 0x00) {
			uint8_t xor_key = buf[0x10];
			for (int i = 0; i < v6v6555v6v6_found - buf.begin(); ++i) {
				buf[i] ^= xor_key; // XOR the buffer with xor_key until v6v6555v6v6 is reached
			}
		}
		parse_post_v6(v6v6555v6v6_found, buf);
	} else {
		if (buf[0x10] != 0) {
			uint8_t xor_key = buf[0x10];
			for (int i = 0; i < buf.end() - buf.begin(); ++i) {
				buf[i] ^= xor_key; // XOR the buffer with xor_key until the end of the buffer as no v6v6555v6v6
			}
		}
	}

	uint32_t main_data_offset = *reinterpret_cast<uint32_t *>(&buf[0x20]);
	uint32_t net_data_offset  = *reinterpret_cast<uint32_t *>(&buf[0x28]);

	uint32_t main_data_start = main_data_offset + 0x20;
	uint32_t net_data_start  = net_data_offset + 0x20;

	uint32_t main_data_blocks_size = *reinterpret_cast<uint32_t *>(&buf[main_data_start]);
	uint32_t net_block_size        = *reinterpret_cast<uint32_t *>(&buf[net_data_start]);

	std::vector<char> net_block_buf(buf.begin() + net_data_start + 4, buf.begin() + net_data_start + net_block_size + 4);
	parse_net_block(net_block_buf);

	uint32_t current_pointer = main_data_start + 4;
	while (current_pointer < main_data_start + 4 + main_data_blocks_size) {
		uint8_t block_type = buf[current_pointer];
		current_pointer += 1;
		uint32_t block_size = *reinterpret_cast<uint32_t *>(&buf[current_pointer]);
		current_pointer += 4;
		std::vector<char> block_buf(buf.begin() + current_pointer, buf.begin() + current_pointer + block_size);
		process_block(block_type, block_buf);
		current_pointer += block_size;
	}
	find_xy_translation();
	translate_segments();
	translate_pins();

	valid = true;

	num_parts  = parts.size();
	num_pins   = pins.size();
	num_format = format.size();
	num_nails  = nails.size();
}

void XZZPCBFile::process_block(uint8_t block_type, std::vector<char> &block_buf) {
	switch (block_type) {
		case 0x01: { // ARC
			std::vector<uint32_t> arc_data((uint32_t *)(block_buf.data()), (uint32_t *)(block_buf.data() + block_buf.size()));
			parse_arc_block(arc_data);
			break;
		}
		case 0x02: { // VIA
			         // Not currently relevant
			break;
		}
		case 0x05: { // LINE SEGMENT
			std::vector<uint32_t> line_segment_data((uint32_t *)(block_buf.data()),
			                                        (uint32_t *)(block_buf.data() + block_buf.size()));
			parse_line_segment_block(line_segment_data);
			break;
		}
		case 0x06: { // TEXT
			// Not currently relevant
			break;
		}
		case 0x07: { // PART/PIN
			std::vector<char> part_data(block_buf.begin(), block_buf.end());
			parse_part_block(part_data);
			break;
		}
		case 0x09: { // TEST PADS/DRILL HOLES
			std::vector<uint8_t> test_pad_data((uint8_t *)(block_buf.data()), (uint8_t *)(block_buf.data() + block_buf.size()));
			parse_test_pad_block(test_pad_data);
			break;
		}
		default: break;
	}
}

char XZZPCBFile::read_utf8_char(char c) const {
	if (static_cast<unsigned char>(c) < 128) {
		return c; // Print ASCII character as is
	} else {
		return '?'; // Replace non-ASCII character with '?'
	}
}

std::string XZZPCBFile::read_cb2312_string(const std::string &str) {
	std::string result;
	bool last_was_cb2312 = false; // As CB2312 encoded characters are 2 bytes each so single '?' can represent one CB2312 character
	for (char c : str) {
		if (static_cast<unsigned char>(c) < 128) {
			result += c;
			last_was_cb2312 = false;
		} else {
			if (last_was_cb2312) {
				result += '?';
				last_was_cb2312 = false;
			} else {
				last_was_cb2312 = true;
			}
		}
	}
	return result;
}

// atm some diode readings aren't processed properly
void XZZPCBFile::parse_post_v6(std::vector<char>::iterator v6_pos, std::vector<char> &buf) {
	unsigned int current_pointer = v6_pos - buf.begin() + 11;
	current_pointer += 7; // While post v6 isnt handled properly
	if (buf[current_pointer] == 0x0A) {
		// Type 1
		// 0x0A '=480=N65594(1)'
		diode_readings_type = 1;
		while (current_pointer < buf.size()) {
			current_pointer += 1;
			if (current_pointer >= buf.size()) {
				return;
			}
			current_pointer += 1; // =
			std::string volt_reading = "";
			while (buf[current_pointer] != 0x3D) {
				volt_reading += buf[current_pointer];
				current_pointer += 1;
			}
			volt_reading = read_cb2312_string(volt_reading);
			current_pointer += 1;
			std::string net = "";
			while (buf[current_pointer] != 0x28) {
				net += buf[current_pointer];
				current_pointer += 1;
			}
			net = read_cb2312_string(net);
			current_pointer += 1;
			std::string pin_name = "";
			while (buf[current_pointer] != 0x29) {
				pin_name += buf[current_pointer];
				current_pointer += 1;
			}
			pin_name = read_cb2312_string(pin_name);
			current_pointer += 1;

			diode_dict[net][pin_name] = volt_reading;
		}
	} else {
		if (buf[current_pointer] != 0x0D) {
			// Type 2
			// 0xCD 0xBC 0x0D 0x0A 'Net242=0'
			// printf("Type 2 Diode Reading\n");
			current_pointer += 2; // Currenlty unknown what these two bytes are
		} else {
			// Type 3
			// 0x0D 0x0A 'SMBUS_SMC_5_G3_SCL=0.5'
			// printf("Type 3 Diode Reading\n");
		}
		diode_readings_type = 2;

		while (current_pointer < buf.size()) {
			current_pointer += 2;
			if (current_pointer >= buf.size()) {
				return;
			}
			if (buf[current_pointer] == 0x0D) {
				break; // Has done 0x0D 0x0A 0x0D 0x0A so end of block
			}
			std::string net = "";
			while (buf[current_pointer] != 0x3D) {
				net += buf[current_pointer];
				current_pointer += 1;
			}
			net = read_cb2312_string(net);
			current_pointer += 1;
			std::string comment = "";
			while (buf[current_pointer] != 0x0D) {
				comment += buf[current_pointer];
				current_pointer += 1;
			}
			comment              = read_cb2312_string(comment);
			diode_dict[net]["0"] = comment; // Seemingly is there is a second reading the first is replaced
		}
	}
}

// Layers:
// 1->16 Trace Layers (Used in order excluding last which always uses 16)
// 17 Silkscreen
// 18->27 Unknown
// 28 Board edges

void XZZPCBFile::parse_arc_block(std::vector<uint32_t> &buf) {
	uint32_t layer      = buf[0];
	uint32_t x          = buf[1];
	uint32_t y          = buf[2];
	int32_t r           = buf[3];
	int32_t angle_start = buf[4];
	int32_t angle_end   = buf[5];
	int32_t scale       = buf[6];
	// int32_t unknown_arc = buf[7];
	scale = 10000;
	if (layer != 28) {
		return;
	}

	int point_x     = static_cast<int>(x / scale);
	int point_y     = static_cast<int>(y / scale);
	r               = r / scale;
	angle_start     = angle_start / scale;
	angle_end       = angle_end / scale;
	BRDPoint centre = {point_x, point_y};

	std::vector<std::pair<BRDPoint, BRDPoint>> segments = xzz_arc_to_segments(angle_start, angle_end, r, centre);
	std::move(segments.begin(), segments.end(), std::back_inserter(outline_segments));
}

void XZZPCBFile::parse_line_segment_block(std::vector<uint32_t> &buf) {
	int32_t layer           = buf[0];
	int32_t x1              = buf[1];
	int32_t y1              = buf[2];
	int32_t x2              = buf[3];
	int32_t y2              = buf[4];
	int32_t scale           = buf[5];
	scale                   = 10000;
	// int32_t trace_net_index = buf[6];	/* unused */
	if (layer != 28) {
		return;
	}

	BRDPoint point;
	point.x = static_cast<int>(static_cast<double>(x1) / static_cast<double>(scale));
	point.y = static_cast<int>(static_cast<double>(y1) / static_cast<double>(scale));
	BRDPoint point2;
	point2.x = static_cast<int>(static_cast<double>(x2) / static_cast<double>(scale));
	point2.y = static_cast<int>(static_cast<double>(y2) / static_cast<double>(scale));
	outline_segments.push_back({point, point2});
}

void XZZPCBFile::parse_part_block(std::vector<char> &buf) {
	BRDPart blank_part;
	BRDPin blank_pin;
	BRDPart part;
	BRDPin pin;

	des_decrypt(buf);

	uint32_t current_pointer = 0;
	uint32_t part_size       = *reinterpret_cast<uint32_t *>(&buf[current_pointer]);
	current_pointer += 4;
	current_pointer += 18;
	uint32_t part_group_name_size = *reinterpret_cast<uint32_t *>(&buf[current_pointer]);
	current_pointer += 4;
	current_pointer += part_group_name_size;

	// So far 0x06 sub blocks have been first always
	// Also contains part name so needed before pins
	if (buf[current_pointer] != 0x06) {
		exit(1);
	}

	current_pointer += 31;
	uint32_t part_name_size = *reinterpret_cast<uint32_t *>(&buf[current_pointer]);
	current_pointer += 4;
	std::string part_name(reinterpret_cast<char *>(&buf[current_pointer]), part_name_size);
	current_pointer += part_name_size;

	part.name          = strdup(part_name.c_str());
	part.mounting_side = BRDPartMountingSide::Top;
	part.part_type     = BRDPartType::SMD;

	// uint32_t pin_count = 0; /* currently unused */
	while (current_pointer <= part_size) {
		uint8_t sub_type_identifier = buf[current_pointer];
		current_pointer += 1;

		switch (sub_type_identifier) {
			case 0x01: {
				// Currently unsure what this is
				current_pointer += *reinterpret_cast<uint32_t *>(&buf[current_pointer]) + 4; // Skip the block
				break;
			}
			case 0x05: { // Line Segment
				// Not currently relevant for BRDPin
				current_pointer += *reinterpret_cast<uint32_t *>(&buf[current_pointer]) + 4; // Skip the block
				break;
			}
			case 0x06: { // Labels/Part Names
				// Not currently relevant for BRDPin
				current_pointer += *reinterpret_cast<uint32_t *>(&buf[current_pointer]) + 4; // Skip the block
				break;
			}
			case 0x09: { // Pins
				// pin_count += 1;
				pin.side = BRDPinSide::Top;

				// Block size
				uint32_t pin_block_size = *reinterpret_cast<uint32_t *>(&buf[current_pointer]);
				uint32_t pin_block_end  = current_pointer + pin_block_size + 4;
				current_pointer += 4;
				current_pointer += 4; // currently unknown

				pin.pos.x = *reinterpret_cast<uint32_t *>(&buf[current_pointer]) / 10000;
				current_pointer += 4;
				pin.pos.y = *reinterpret_cast<uint32_t *>(&buf[current_pointer]) / 10000;
				current_pointer += 4;
				current_pointer += 8; // currently unknown

				uint32_t pin_name_size = *reinterpret_cast<uint32_t *>(&buf[current_pointer]);
				current_pointer += 4;
				std::string pin_name(reinterpret_cast<char *>(&buf[current_pointer]), pin_name_size);

				pin.name = strdup(pin_name.c_str());
				pin.snum = strdup(pin_name.c_str());
				current_pointer += pin_name_size;
				current_pointer += 32;

				uint32_t net_index = *reinterpret_cast<uint32_t *>(&buf[current_pointer]);
				current_pointer    = pin_block_end;

				std::string diode_reading;

				std::string pin_net = net_dict[net_index];

				if (pin_net == "NC") {
					pin.net = strdup("UNCONNECTED");
				} else {
					pin.net = strdup(pin_net.c_str());
				}

				pin.part = parts.size() + 1;

				if (!diode_reading.empty()) {
					pin.comment = strdup(diode_reading.c_str());
				} else if (diode_readings_type == 1) {
					if (diode_dict.find(part.name) != diode_dict.end() &&
					    diode_dict[part.name].find(pin.name) != diode_dict[part.name].end()) {
						pin.comment = strdup(diode_dict[part.name][pin.name].c_str());
					}
				} else if (diode_readings_type == 2) {
					if (diode_dict.find(pin.net) != diode_dict.end()) {
						pin.comment = strdup(diode_dict[pin.net]["0"].c_str());
					}
				}

				pins.push_back(pin);
				pin = blank_pin;

				break;
			}
			default:
				if (sub_type_identifier != 0x00) {
					printf("Unknown sub block type: 0x%02X at %d in %s\n", sub_type_identifier, current_pointer, part_name.c_str());
				}
				break;
		}
	}

	part.end_of_pins = pins.size();
	parts.push_back(part);
	part = blank_part;
}

void XZZPCBFile::parse_test_pad_block(std::vector<uint8_t> &buf) {
	BRDPart blank_part;
	BRDPin blank_pin;
	BRDPart part;
	BRDPin pin;

	uint32_t current_pointer = 0;
	// uint32_t pad_number      = *reinterpret_cast<uint32_t *>(&buf[current_pointer]); /* unused */
	current_pointer += 4;
	uint32_t x_origin = *reinterpret_cast<uint32_t *>(&buf[current_pointer]);
	current_pointer += 4;
	uint32_t y_origin = *reinterpret_cast<uint32_t *>(&buf[current_pointer]);
	current_pointer += 4;
	current_pointer += 8; // inner_diameter + unknown1
	uint32_t name_length = *reinterpret_cast<uint32_t *>(&buf[current_pointer]);
	current_pointer += 4;
	std::string name(reinterpret_cast<char *>(&buf[current_pointer]), name_length);
	current_pointer += name_length;
	current_pointer    = buf.size() - 4;
	uint32_t net_index = *reinterpret_cast<uint32_t *>(&buf[current_pointer]);

	part.name          = strdup(("..." + name).c_str()); // To make it get the kPinTypeTestPad type
	part.mounting_side = BRDPartMountingSide::Top;
	part.part_type     = BRDPartType::SMD;

	pin.snum  = strdup(name.c_str());
	pin.side  = BRDPinSide::Top;
	pin.pos.x = static_cast<int>(static_cast<double>(x_origin / 10000.0));
	pin.pos.y = static_cast<int>(static_cast<double>(y_origin / 10000.0));
	if (net_dict.find(net_index) != net_dict.end()) {
		if (net_dict[net_index] == "UNCONNECTED" || net_dict[net_index] == "NC") {
			pin.net = strdup(""); // As the part already gets the kPinTypeTestPad type if "UNCONNECTED" is used type will be changed
			                      // to kPinTypeNotConnected
		} else {
			pin.net = strdup(net_dict[net_index].c_str());
		}
	} else {
		pin.net = strdup(""); // As the part already gets the kPinTypeTestPad type if "UNCONNECTED" is used type will be changed to
		                      // kPinTypeNotConnected
	}
	pin.part = parts.size() + 1;
	pins.push_back(pin);
	pin              = blank_pin;
	part.end_of_pins = pins.size();
	parts.push_back(part);
	part = blank_part;
}

void XZZPCBFile::parse_net_block(std::vector<char> &buf) {
	uint32_t current_pointer = 0;
	while (current_pointer < buf.size()) {
		uint32_t net_size = *reinterpret_cast<uint32_t *>(&buf[current_pointer]);
		current_pointer += 4;
		uint32_t net_index = *reinterpret_cast<uint32_t *>(&buf[current_pointer]);
		current_pointer += 4;
		std::string net_name(&buf[current_pointer], net_size - 8);
		current_pointer += net_size - 8;

		net_dict[net_index] = net_name;
	}
}

// Translation and mirroring functions
void XZZPCBFile::find_xy_translation() {
	// Assuming line segments encompass all parts
	// Find the min and max x and y values

	if (outline_segments.empty()) {
		xy_translation.x = 0;
		xy_translation.y = 0;
		return;
	}
	xy_translation.x = outline_segments[0].first.x;
	xy_translation.y = outline_segments[0].first.y;
	for (auto &segment : outline_segments) {
		xy_translation.x = std::min({xy_translation.x, segment.first.x, segment.second.x});
		xy_translation.y = std::min({xy_translation.y, segment.first.y, segment.second.y});
	}
}

void XZZPCBFile::translate_points(BRDPoint &point) const {
	point.x -= xy_translation.x;
	point.y -= xy_translation.y;
}

void XZZPCBFile::translate_segments() {
	for (auto &segment : outline_segments) {
		translate_points(segment.first);
		translate_points(segment.second);
	}
}

void XZZPCBFile::translate_pins() {
	for (auto &pin : pins) {
		translate_points(pin.pos);
	}
}
