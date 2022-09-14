#include "BVR3File.h"

#include "utils.h"
#include <cmath>
#include <cctype>
#include <clocale>
#include <cstdint>
#include <cstring>
#include <list>
#include <algorithm>

int manhattan_distance(const BRDPoint &p1, const BRDPoint &p2) {
	return abs(p1.x - p2.x) + abs(p1.y - p2.y);
}

bool BVR3File::verifyFormat(std::vector<char> &buf) {
	return find_str_in_buf("BVRAW_FORMAT_3", buf);
}

BVR3File::BVR3File(std::vector<char> &buf) {
	auto buffer_size = buf.size();

	char *saved_locale = setlocale(LC_NUMERIC, "C"); // Use '.' as delimiter for strtod

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

	BRDPart blank_part;
	BRDPin blank_pin;
	BRDPart part;
	BRDPin pin;
	std::list<std::pair<BRDPoint, BRDPoint>> outline_segments;

	std::vector<char *> lines;
	stringfile(file_buf, lines);

	for (char *line : lines) {
		while (isspace((uint8_t)*line)) line++;
		if (!line[0]) continue;

		char *p = line;
		char *s;

		if (!strncmp(line, "PART_NAME ", 10)) {
			p += 10;
			part.name = READ_STR();
		} else if (!strncmp(line, "PART_SIDE ", 10)) {
			p += 10;
			char *side = READ_STR();
			if (!strcmp(side, "T"))
				part.mounting_side = BRDPartMountingSide::Top;
			else if (!strcmp(side, "B"))
				part.mounting_side = BRDPartMountingSide::Bottom;
			else if (!strcmp(side, "O"))
				part.mounting_side = BRDPartMountingSide::Both;
		} else if (!strncmp(line, "PART_ORIGIN ", 12)) {
			// Value ignored, used as reference point for relative pin placements, not currently supported
		} else if (!strncmp(line, "PART_MOUNT ", 11)) {
			p += 11;
			char *mount = READ_STR();
			if (!strcmp(mount, "SMD"))
				part.part_type = BRDPartType::SMD;
			else
				part.part_type = BRDPartType::ThroughHole;
		} else if (!strncmp(line, "PART_OUTLINE_RELATIVE ", 22)) {
			// Value ignored, custom outline for parts not yet supported
		} else if (!strncmp(line, "PIN_ID ", 7)) {
			// Value ignored, not currently relevant for BRDPin
		} else if (!strncmp(line, "PIN_NUMBER ", 11)) {
			p += 11;
			pin.snum = READ_STR();
		} else if (!strncmp(line, "PIN_NAME ", 9)) {
			p += 9;
			pin.name = READ_STR();
		} else if (!strncmp(line, "PIN_SIDE ", 9)) {
			p += 9;
			char *side = READ_STR();
			if (!strcmp(side, "T"))
				pin.side = BRDPinSide::Top;
			else if (!strcmp(side, "B"))
				pin.side = BRDPinSide::Bottom;
			else if (!strcmp(side, "O"))
				pin.side = BRDPinSide::Both;
		} else if (!strncmp(line, "PIN_ORIGIN ", 11)) {
			p += 11;
			double origin_x = READ_DOUBLE();
			pin.pos.x = trunc(origin_x);
			double origin_y = READ_DOUBLE();
			pin.pos.y = trunc(origin_y);
		} else if (!strncmp(line, "PIN_RADIUS ", 11)) {
			p += 11;
			pin.radius = READ_DOUBLE();
		} else if (!strncmp(line, "PIN_NET ", 8)) {
			p += 8;
			pin.net = READ_STR();
		} else if (!strncmp(line, "PIN_TYPE ", 9)) {
			// Value ignored, not currently relevant for BRDPin
		} else if (!strncmp(line, "PIN_COMMENT ", 12)) {
			// Value ignored, not currently relevant for BRDPin
		} else if (!strncmp(line, "PIN_OUTLINE_RELATIVE ", 21)) {
			// Value ignored, custom outline for pins not yet supported
		} else if (!strcmp(line, "PIN_END")) {
			pin.part = parts.size() + 1; // pin is for current part, which will not yet have been added to parts vector
			pins.push_back(pin);
			pin = blank_pin;
		} else if (!strcmp(line, "PART_END")) {
			part.end_of_pins = pins.size();
			parts.push_back(part);
			part = blank_part;
		} else if (!strncmp(line, "OUTLINE_POINTS ", 15)) {
			p += 15;
			while (p[0]) {
				auto pold = p;
				BRDPoint point;
				double x = READ_DOUBLE();
				point.x  = trunc(x);
				double y = READ_DOUBLE();
				point.y  = trunc(y);
				// Nothing was read, probably end of list
				if (pold == p) {
					break;
				}
				format.push_back(point);
			}
		} else if (!strncmp(line, "OUTLINE_SEGMENTED ", 18)) {
			p += 18;
			while (p[0]) {
				auto pold = p;
				std::pair<BRDPoint, BRDPoint> outline_segment;
				double x = READ_DOUBLE();
				outline_segment.first.x  = trunc(x);
				double y = READ_DOUBLE();
				outline_segment.first.y  = trunc(y);
				x = READ_DOUBLE();
				outline_segment.second.x = trunc(x);
				y = READ_DOUBLE();
				outline_segment.second.y = trunc(y);
				// Nothing was read, probably end of list
				if (pold == p) {
					break;
				}
				outline_segments.push_back(outline_segment);
			}

			if (outline_segments.empty())
				continue;

			// Get first segment and add both points to format
			auto first_outline_segment = outline_segments.front();
			outline_segments.pop_front();
			format.push_back(first_outline_segment.first);
			format.push_back(first_outline_segment.second);

			// Loop through remaining segments picking the best candidate for the next segment
			BRDPoint start_point = first_outline_segment.first;
			BRDPoint end_point = first_outline_segment.second;
			while (start_point != end_point && !outline_segments.empty()) {
				// Loop through segments checking for exact match between points
				auto it = outline_segments.begin();
				bool match_found = false;
				while (it != outline_segments.end()) {
					// If exact match found on either first or second point, add other point to format and remove segment
					if (end_point == it->first) {
						format.push_back(it->second);
						end_point = it->second;
						outline_segments.erase(it);
						match_found = true;
						break;
					} else if (end_point == it->second) {
						format.push_back(it->first);
						end_point = it->first;
						outline_segments.erase(it);
						match_found = true;
						break;
					}
					it++;
				}
				if (match_found)
					continue;

				// Exact match not found, so pick nearest segment instead
				it = std::min_element(
					outline_segments.begin(),
					outline_segments.end(),
					[&end_point](const std::pair<BRDPoint, BRDPoint> &os1, const std::pair<BRDPoint, BRDPoint> &os2) {
						return std::min(manhattan_distance(end_point, os1.first), manhattan_distance(end_point, os1.second))
							< std::min(manhattan_distance(end_point, os2.first), manhattan_distance(end_point, os2.second));
					});

				// Calculate distances for comparison
				auto start_point_distance = manhattan_distance(end_point, start_point);
				auto segment_distance_first = manhattan_distance(end_point, it->first);
				auto segment_distance_second = manhattan_distance(end_point, it->second);

				// If start point is closer than both nearest segment points, format path more likely to be complete
				if (start_point_distance <= segment_distance_first && start_point_distance <= segment_distance_second) {
					format.push_back(start_point);
					end_point = start_point;
					break;
				}

				// Otherwise add points from nearest segment to format with closest point first, then remove segment
				if (segment_distance_first <= segment_distance_second) {
					format.push_back(it->first);
					format.push_back(it->second);
					end_point = it->second;
					outline_segments.erase(it);
				} else {
					format.push_back(it->second);
					format.push_back(it->first);
					end_point = it->first;
					outline_segments.erase(it);
				}
			}
		}
	}

	num_parts  = parts.size();
	num_pins   = pins.size();
	num_format = format.size();
	num_nails  = nails.size();

	setlocale(LC_NUMERIC, saved_locale); // Restore locale

	valid = num_parts > 0 || num_format > 0;
}
