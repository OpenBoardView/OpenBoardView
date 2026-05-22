#pragma once

#include "BRDFileBase.h"

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct XZZPCBFile : public BRDFileBase {
  public:
	XZZPCBFile(std::vector<char> &buf, uint64_t key);

	static bool verifyFormat(const std::vector<char> &buf);

  private:
	uint64_t key = 0ul;
	static const int XZZ_GLOBAL_SCALE = 10000;
	std::unordered_map<uint32_t, std::string> net_dict;
	std::unordered_map<std::string, std::unordered_map<std::string, std::string>> diode_dict; // <Net Name, <Pin Name, Reading>>

	const std::array<uint8_t, 8> getKeyParity() const;
	bool checkKey(uint64_t key) const;
	std::string keyToString(uint64_t key) const;

	// DES
	std::vector<char> des_decrypt(const std::vector<char> &buf);

	std::vector<std::pair<BRDPoint, BRDPoint>> xzz_arc_to_segments(int startAngle, int endAngle, int r, BRDPoint pc);
	void parse_arc_block(const std::vector<char> &buf);
	void parse_line_segment_block(const std::vector<char> &buf);
	BRDPin parse_pin_block(const std::vector<char> &buf, uint32_t &current_pointer);
	void parse_part_block(std::vector<char> &encrypted_buf);
	void parse_test_pad_block(const std::vector<char> &buf);
	void parse_net_block(const std::vector<char> &buf);
	void process_block(std::vector<char> &block_buf, uint8_t block_type);
	void process_blocks(const std::vector<char> &buf, uint32_t main_data_start, uint32_t main_data_blocks_size);

	BRDPoint find_xy_translation() const;
	void translate_segments(const BRDPoint &xy_translation);
	void translate_pins(const BRDPoint &xy_translation);
};
