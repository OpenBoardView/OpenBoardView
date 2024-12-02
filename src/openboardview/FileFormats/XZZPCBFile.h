#pragma once

#include "BRDFileBase.h"
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>
#include "annotations.h"

struct XZZPCBFile : public BRDFileBase {
  public:
	XZZPCBFile(std::vector<char> &buf, std::string filepath);

	static bool verifyFormat(std::vector<char> &buf);

  private:
	std::unordered_map<uint32_t, std::string> net_dict;
	std::unordered_map<std::string, std::unordered_map<std::string, std::string>> diode_dict; // <Net Name, <Pin Name, Reading>>
	BRDPoint xy_translation = {0, 0};
	int diode_readings_type = 0; // 0 = No readings, 1 = Based on part name and pin name, 2 = Based on net

	// DES
	uint64_t des(uint64_t input, uint64_t key, char mode);
	void des_decrypt(std::vector<char> &buf);
    void decryptWithDES(std::vector<char> &buf);

	std::vector<std::pair<BRDPoint, BRDPoint>> xzz_arc_to_segments(int startAngle, int endAngle, int r, BRDPoint pc);
	void parse_arc_block(std::vector<uint32_t> &buf);
	void parse_line_segment_block(std::vector<uint32_t> &buf);
	void parse_part_block(std::vector<char> &buf);
	void parse_test_pad_block(std::vector<uint8_t> &buf);
	void parse_post_v6(std::vector<char>::iterator v6_pos, std::vector<char> &buf);
	void parse_net_block(std::vector<char> &buf);
	void process_block(uint8_t block_type, std::vector<char> &block_buf);

    char read_utf8_char(char c);
	std::string read_cb2312_string(const std::string &str);

	void find_xy_translation();
    void translate_segments();
	void translate_points(BRDPoint &point);
	void translate_pins();
};