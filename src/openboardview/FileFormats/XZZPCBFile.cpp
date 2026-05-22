#include "XZZPCBFile.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <list>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Crypto/des.h"
#include "utils.h"

/*
 * Credit to @huertas for DES functions
 * Also credit to @inflex and @MuertoGB for help with cracking the encryption + decoding the format
 */


static inline uint32_t read_uint32_t(const std::vector<char> &buf, size_t start_pos, std::string &error_msg) {
	ENSURE_OR_FAIL(buf.size() > start_pos + 3, error_msg, return 0);
	return ((static_cast<uint32_t>(static_cast<unsigned char>(buf[start_pos + 3])) << 24) |
			(static_cast<uint32_t>(static_cast<unsigned char>(buf[start_pos + 2])) << 16) |
			(static_cast<uint32_t>(static_cast<unsigned char>(buf[start_pos + 1])) <<  8) |
			(static_cast<uint32_t>(static_cast<unsigned char>(buf[start_pos + 0])) <<  0));
}

std::vector<char> XZZPCBFile::des_decrypt(const std::vector<char> &inbuf) {
	std::vector<char> outbuf(inbuf.size());

	// Iterate over input and output buffer at the same time by chunks of 8 bytes
	auto inpos = inbuf.begin();
	for (auto outpos = outbuf.begin();
			inpos < inbuf.end() && outpos < outbuf.end();
			inpos += sizeof(uint64_t), outpos += sizeof(uint64_t)) {
		// Convert 8 bytes of the input buffer into a 64-bit unsigned int with byte order reversed
		uint64_t input = 0l;
		for (size_t i = 0; i < sizeof(uint64_t) && inpos + i < inbuf.end(); i++) {
			input |= static_cast<uint64_t>(static_cast<unsigned char>(inpos[sizeof(uint64_t) - 1 - i])) << (i * 8);
		}

		uint64_t output = des(input, key, 'd');

		// Convert the resulting 64-bit unsigned back into 8 bytes in the output buffer with byte order reversed
		for (size_t i = 0; i < sizeof(uint64_t) && outpos + i < outbuf.end(); i++) {
			outpos[sizeof(uint64_t) - 1 - i] = (output >> (i * 8)) & 0xff;
		}
	}

	return outbuf;
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

	BRDPoint pold = {static_cast<int>(pc_xD + rD * std::cos(startAngleD)),
	                 static_cast<int>(pc_yD + rD * std::sin(startAngleD))};
	for (int i = 1; i < numPoints; ++i) {
		double angle = startAngleD + i * angleStep;
		BRDPoint p   = {static_cast<int>(pc_xD + rD * std::cos(angle)), static_cast<int>(pc_yD + rD * std::sin(angle))};
		arc_segments.push_back({pold, p});
		pold = p;
	}

	return arc_segments;
}

bool XZZPCBFile::checkKey(uint64_t key) const {
	auto key_parity = getKeyParity();
	bool valid_key = true;
	for (size_t i = 0; i < sizeof(uint64_t); i++) { // Compute parity for each byte of XZZ key
		uint8_t tmp = (key >> (i * 8)) & 0xff;
		tmp ^= tmp >> 4;
		tmp ^= tmp >> 2;
		tmp ^= tmp >> 1;
		tmp = (~tmp) & 1;
		valid_key = valid_key && (tmp == key_parity[i]);
	}
	return valid_key;
}

const std::array<uint8_t, 8> XZZPCBFile::getKeyParity() const {
	return {{1, 1, 1, 1, 1, 1, 1, 0}};
}

std::string XZZPCBFile::keyToString(uint64_t key) const {
	std::stringstream ss;
	ss << "0x" << std::setfill('0') << std::setw(sizeof(key) * 2)  << std::hex << key;
	return ss.str();
}

bool XZZPCBFile::verifyFormat(const std::vector<char> &buf) {
	if (buf.size() < 6) {
		return false;
	}

	if (std::string(buf.begin(), buf.begin() + 6) == "XZZPCB") {
		return true;
	}

	if (buf.size() > 0x10 && buf[0x10] != 0x00) {
		uint8_t xor_key = buf[0x10];
		std::vector<char> xor_buf(buf.begin(), buf.begin() + 6);
		for (size_t i = 0; i < 6; ++i) {
			xor_buf[i] ^= xor_key;
		}
		return std::string(xor_buf.begin(), xor_buf.end()) == "XZZPCB";
	}

	return false;
}

XZZPCBFile::XZZPCBFile(std::vector<char> &buf, uint64_t xzzkey) {
	std::list<std::pair<BRDPoint, BRDPoint>> outline_segments;

	if (checkKey(xzzkey)) {
		key = xzzkey;
	} else if (!checkKey(key)) { // Try to fallback to built-in key
		valid = false;
		error_msg = "Invalid XZZ PCB Key\nXZZ PCB key: " + keyToString(xzzkey);
		return;
	}

	std::string v6v6555v6v6{"v6v6555v6v6"};
	auto v6v6555v6v6_found = std::search(buf.begin(), buf.end(), v6v6555v6v6.begin(), v6v6555v6v6.end());

	// v6v6555v6v6_found is buf.end() if not found
	ENSURE_OR_FAIL(buf.size() >= 0x10, error_msg, return);
	if (buf[0x10] != 0x00) {
		uint8_t xor_key = buf[0x10];
		for (auto pos = buf.begin(); pos < v6v6555v6v6_found; pos++) {
			*pos ^= xor_key; // XOR the buffer with xor_key until v6v6555v6v6 is reached
		}
	}

	uint32_t main_data_offset = read_uint32_t(buf, 0x20, error_msg);
	uint32_t net_data_offset  = read_uint32_t(buf, 0x28, error_msg);

	uint32_t main_data_start = main_data_offset + 0x20;
	uint32_t net_data_start  = net_data_offset + 0x20;

	uint32_t main_data_blocks_size = read_uint32_t(buf, main_data_start, error_msg);
	uint32_t net_block_size        = read_uint32_t(buf, net_data_start, error_msg);

	if (!error_msg.empty()) { // Check if one of read_uint32_t() failed
		return;
	}

	ENSURE_OR_FAIL(buf.size() >= net_data_start + net_block_size + 4, error_msg, return);
	std::vector<char> net_block_buf(buf.begin() + net_data_start + 4, buf.begin() + net_data_start + net_block_size + 4);
	parse_net_block(net_block_buf);
	if (!error_msg.empty()) { // Check if parse_net_block() failed
		return;
	}

	process_blocks(buf, main_data_start, main_data_blocks_size);
	if (!error_msg.empty()) { // Check if process_blocks() failed
		return;
	}

	BRDPoint xy_translation = find_xy_translation();
	translate_segments(xy_translation);
	translate_pins(xy_translation);

	valid = true;

	num_parts  = parts.size();
	num_pins   = pins.size();
	num_format = format.size();
	num_nails  = nails.size();
}

void XZZPCBFile::process_block(std::vector<char> &block_buf, uint8_t block_type) {
	switch (block_type) {
		case 0x01: // ARC
			parse_arc_block(block_buf);
			break;
		case 0x02: // VIA
			// Not currently relevant
			break;
		case 0x05: // LINE SEGMENT
			parse_line_segment_block(block_buf);
			break;
		case 0x06: // TEXT
			// Not currently relevant
			break;
		case 0x07: // PART/PIN
			parse_part_block(block_buf);
			break;
		case 0x09: // TEST PADS/DRILL HOLES
			parse_test_pad_block(block_buf);
			break;
		default:
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "XZZPCBFile: Unhandled block type: %x", block_type);
			break;
	}
}

void XZZPCBFile::process_blocks(const std::vector<char> &buf, uint32_t main_data_start, uint32_t main_data_blocks_size) {
	ENSURE_OR_FAIL(buf.size() >= main_data_start + 4 + main_data_blocks_size, error_msg, return);
	uint32_t current_pointer = main_data_start + 4;
	while (current_pointer < main_data_start + 4 + main_data_blocks_size) {
		uint8_t block_type = buf[current_pointer];
		current_pointer += 1;
		uint32_t block_size = read_uint32_t(buf, current_pointer, error_msg);
		current_pointer += 4;
		if (!error_msg.empty()) { // Check if read_uint32_t() failed
			return;
		}

		ENSURE_OR_FAIL(buf.size() >= current_pointer + block_size, error_msg, return);
		std::vector<char> block_buf(buf.begin() + current_pointer, buf.begin() + current_pointer + block_size);
		process_block(block_buf, block_type);
		current_pointer += block_size;
		if (!error_msg.empty()) { // Check if process_block() failed
			return;
		}
	}
}

// Layers:
// 1->16 Trace Layers (Used in order excluding last which always uses 16)
// 17 Silkscreen
// 18->27 Unknown
// 28 Board edges

void XZZPCBFile::parse_arc_block(const std::vector<char> &buf) {
	uint32_t layer       = read_uint32_t(buf, 0 * sizeof(uint32_t), error_msg);
	uint32_t x           = read_uint32_t(buf, 1 * sizeof(uint32_t), error_msg);
	uint32_t y           = read_uint32_t(buf, 2 * sizeof(uint32_t), error_msg);
	uint32_t r           = read_uint32_t(buf, 3 * sizeof(uint32_t), error_msg);
	uint32_t angle_start = read_uint32_t(buf, 4 * sizeof(uint32_t), error_msg);
	uint32_t angle_end   = read_uint32_t(buf, 5 * sizeof(uint32_t), error_msg);
	uint32_t scale       = read_uint32_t(buf, 6 * sizeof(uint32_t), error_msg);
	// int32_t unknown_arc = read_int32_t(buf, 7, error_msg);
	if (!error_msg.empty()) { // Check if one of read_*int32_t() failed
		return;
	}

	scale = XZZ_GLOBAL_SCALE;
	if (layer != 28) {
		return;
	}

	int point_x     = x / scale;
	int point_y     = y / scale;
	r               = r / scale;
	angle_start     = angle_start / scale;
	angle_end       = angle_end / scale;
	BRDPoint centre = {point_x, point_y};

	std::vector<std::pair<BRDPoint, BRDPoint>> segments = xzz_arc_to_segments(angle_start, angle_end, r, centre);
	std::move(segments.begin(), segments.end(), std::back_inserter(outline_segments));
}

void XZZPCBFile::parse_line_segment_block(const std::vector<char> &buf) {
	uint32_t layer           = read_uint32_t(buf, 0 * sizeof(uint32_t), error_msg);
	uint32_t x1              = read_uint32_t(buf, 1 * sizeof(uint32_t), error_msg);
	uint32_t y1              = read_uint32_t(buf, 2 * sizeof(uint32_t), error_msg);
	uint32_t x2              = read_uint32_t(buf, 3 * sizeof(uint32_t), error_msg);
	uint32_t y2              = read_uint32_t(buf, 4 * sizeof(uint32_t), error_msg);
	uint32_t scale           = read_uint32_t(buf, 5 * sizeof(uint32_t), error_msg);
	// uint32_t trace_net_index = read_uint32_t(buf, 6, error_msg);	/* unused */
	if (!error_msg.empty()) { // Check if one of read_int32_t() failed
		return;
	}

	scale = XZZ_GLOBAL_SCALE;
	if (layer != 28) {
		return;
	}

	BRDPoint point;
	point.x = x1 / scale;
	point.y = y1 / scale;
	BRDPoint point2;
	point2.x = x2 / scale;
	point2.y = y2 / scale;
	outline_segments.push_back({point, point2});
}

BRDPin XZZPCBFile::parse_pin_block(const std::vector<char> &buf, uint32_t &current_pointer) {
	BRDPin pin{};
	pin.side = BRDPinSide::Top;

	// Block size
	uint32_t pin_block_size = read_uint32_t(buf, current_pointer, error_msg);
	uint32_t pin_block_end  = current_pointer + pin_block_size + 4;
	current_pointer += 4;
	current_pointer += 4; // currently unknown

	uint32_t x_origin = read_uint32_t(buf, current_pointer, error_msg);
	current_pointer += 4;
	uint32_t y_origin = read_uint32_t(buf, current_pointer, error_msg);
	current_pointer += 4;
	current_pointer += 8; // currently unknown

	uint32_t pin_name_size = read_uint32_t(buf, current_pointer, error_msg);
	current_pointer += 4;
	if (!error_msg.empty()) { // Check if one of read_uint32_t() failed
		return {};
	}

	ENSURE_OR_FAIL(buf.size() >= current_pointer + pin_name_size, error_msg, return {});
	std::string pin_name(buf.begin() + current_pointer, buf.begin() + current_pointer + pin_name_size);
	current_pointer += pin_name_size;
	current_pointer += 32;

	uint32_t net_index = read_uint32_t(buf, current_pointer, error_msg);
	current_pointer    = pin_block_end;
	if (!error_msg.empty()) { // Check if read_uint32_t() failed
		return {};
	}

	pin.pos.x = x_origin / XZZ_GLOBAL_SCALE;
	pin.pos.y = y_origin / XZZ_GLOBAL_SCALE;
	pin.name = strdup(pin_name.c_str());
	pin.snum = pin.name;

	std::string pin_net = net_dict[net_index];

	if (pin_net == "NC") {
		pin.net = "UNCONNECTED";
	} else {
		pin.net = strdup(pin_net.c_str());
	}

	return pin;
}

void XZZPCBFile::parse_part_block(std::vector<char> &encrypted_buf) {
	BRDPart part{};

	auto buf = des_decrypt(encrypted_buf);

	uint32_t current_pointer = 0;
	uint32_t part_size       = read_uint32_t(buf, current_pointer, error_msg);
	current_pointer += 4;
	current_pointer += 18;
	uint32_t part_group_name_size = read_uint32_t(buf, current_pointer, error_msg);
	current_pointer += 4;
	current_pointer += part_group_name_size;

	// So far 0x06 sub blocks have been first always
	// Also contains part name so needed before pins
	ENSURE_OR_FAIL(current_pointer < buf.size(), error_msg, return);
	ENSURE_OR_FAIL(buf[current_pointer] == 0x06, error_msg, return);

	current_pointer += 31;
	uint32_t part_name_size = read_uint32_t(buf, current_pointer, error_msg);
	current_pointer += 4;
	if (!error_msg.empty()) { // Check if one of read_uint32_t() failed
		return;
	}

	ENSURE_OR_FAIL(buf.size() >= current_pointer + part_name_size, error_msg, return);
	std::string part_name(buf.begin() + current_pointer, buf.begin() + current_pointer + part_name_size);
	current_pointer += part_name_size;

	part.name          = strdup(part_name.c_str());
	part.mounting_side = BRDPartMountingSide::Top;
	part.part_type     = BRDPartType::SMD;

	ENSURE_OR_FAIL(buf.size() >= part_size + 4, error_msg, return);
	while (current_pointer < part_size + 4) {
		uint8_t sub_type_identifier = buf[current_pointer];
		current_pointer += 1;

		switch (sub_type_identifier) {
			case 0x01: // Currently unsure what this is
			case 0x05: // Line Segment, Not currently relevant for BRDPin
			case 0x06: // Labels/Part Names, Not currently relevant for BRDPin
				current_pointer += read_uint32_t(buf, current_pointer, error_msg) + 4; // Skip the block
				if (!error_msg.empty()) { // Check if read_uint32_t() failed
					return;
				}
				break;
			case 0x09: { // Pins
				auto pin = parse_pin_block(buf, current_pointer);
				if (!error_msg.empty()) { // Check if parse_pin() failed
					return;
				}
				pin.part = parts.size() + 1;
				pins.push_back(pin);
				break;
			}
			default:
				if (sub_type_identifier != 0x00) {
					SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "XZZPCBFile: Unknown sub block type: 0x%02X at %d in %s",
							sub_type_identifier, current_pointer, part_name.c_str());
				}
				break;
		}
	}

	part.end_of_pins = pins.size();
	parts.push_back(part);
}

void XZZPCBFile::parse_test_pad_block(const std::vector<char> &buf) {
	BRDPart part{};
	BRDPin pin{};

	uint32_t current_pointer = 0;
	// uint32_t pad_number      = read_uint32_t(buf, current_pointer, error_msg); /* unused */
	current_pointer += 4;
	uint32_t x_origin = read_uint32_t(buf, current_pointer, error_msg);
	current_pointer += 4;
	uint32_t y_origin = read_uint32_t(buf, current_pointer, error_msg);
	current_pointer += 4;
	current_pointer += 8; // inner_diameter + unknown1
	uint32_t name_length = read_uint32_t(buf, current_pointer, error_msg);
	current_pointer += 4;
	if (!error_msg.empty()) { // Check if one of read_uint32_t() failed
		return;
	}

	ENSURE_OR_FAIL(buf.size() >= current_pointer + name_length, error_msg, return);
	std::string name(buf.begin() + current_pointer, buf.begin() + current_pointer + name_length);
	current_pointer += name_length;
	current_pointer    = buf.size() - 4;
	uint32_t net_index = read_uint32_t(buf, current_pointer, error_msg);
	if (!error_msg.empty()) { // Check if one of read_uint32_t() failed
		return;
	}

	part.name          = strdup(("..." + name).c_str()); // To make it get the kPinTypeTestPad type
	part.mounting_side = BRDPartMountingSide::Top;
	part.part_type     = BRDPartType::SMD;

	pin.snum  = strdup(name.c_str());
	pin.side  = BRDPinSide::Top;
	pin.pos.x = x_origin / XZZ_GLOBAL_SCALE;
	pin.pos.y = y_origin / XZZ_GLOBAL_SCALE;
	if (net_dict.find(net_index) != net_dict.end()) {
		if (net_dict[net_index] == "UNCONNECTED" || net_dict[net_index] == "NC") {
			pin.net = ""; // As the part already gets the kPinTypeTestPad type if "UNCONNECTED" is used type will be changed
			              // to kPinTypeNotConnected
		} else {
			pin.net = strdup(net_dict[net_index].c_str());
		}
	} else {
		pin.net = ""; // As the part already gets the kPinTypeTestPad type if "UNCONNECTED" is used type will be changed to
		              // kPinTypeNotConnected
	}
	pin.part = parts.size() + 1;
	pins.push_back(pin);
	part.end_of_pins = pins.size();
	parts.push_back(part);
}

void XZZPCBFile::parse_net_block(const std::vector<char> &buf) {
	uint32_t current_pointer = 0;
	while (current_pointer < buf.size()) {
		uint32_t net_size = read_uint32_t(buf, current_pointer, error_msg);
		current_pointer += 4;
		uint32_t net_index = read_uint32_t(buf, current_pointer, error_msg);
		current_pointer += 4;
		if (!error_msg.empty()) { // Check if one of read_uint32_t() failed
			return;
		}

		ENSURE_OR_FAIL(buf.size() >= current_pointer + net_size - 8, error_msg, return);
		std::string net_name(buf.begin() + current_pointer, buf.begin() + current_pointer + net_size - 8);
		current_pointer += net_size - 8;

		net_dict[net_index] = net_name;
	}
}

// Translation and mirroring functions
BRDPoint XZZPCBFile::find_xy_translation() const {
	// Assuming line segments encompass all parts
	// Find the min and max x and y values
	BRDPoint xy_translation{0, 0};

	if (outline_segments.empty()) {
		return xy_translation;
	}
	xy_translation.x = outline_segments[0].first.x;
	xy_translation.y = outline_segments[0].first.y;
	for (auto &segment : outline_segments) {
		xy_translation.x = std::min({xy_translation.x, segment.first.x, segment.second.x});
		xy_translation.y = std::min({xy_translation.y, segment.first.y, segment.second.y});
	}
	return xy_translation;
}

void XZZPCBFile::translate_segments(const BRDPoint &xy_translation) {
	for (auto &segment : outline_segments) {
		segment.first -= xy_translation;
		segment.second -= xy_translation;
	}
}

void XZZPCBFile::translate_pins(const BRDPoint &xy_translation) {
	for (auto &pin : pins) {
		pin.pos -= xy_translation;
	}
}
