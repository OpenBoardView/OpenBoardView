#include "ADFile.h"
#include "utils.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <clocale>
#include <cstdint>
#include <cstring>
#include <vector>

#ifndef FL
#define FL __FILE__, __LINE__
#endif

#define ADFILE_BLOCK_NONE 0
#define ADFILE_BLOCK_OUTLINE 1
#define ADFILE_BLOCK_NETS 2
#define ADFILE_BLOCK_COMPONENTS 3
#define ADFILE_BLOCK_PADS 4
#define ADFILE_BLOCK_TRACKS 5

char *arena;
char *arena_end;

char *read_item(char *p) {
	char *s;
	char *r;

	while ((*p) && (isspace((uint8_t)*p))) ++p;
	s = p;
	while ((*p) && (!isspace((uint8_t)*p)) && (*p != '|')) ++p;
	*p = 0;
	r  = strdup(s);
	*p = '|';
	return fix_to_utf8(r, &arena, arena_end);
}

bool ADFile::verifyFormat(std::vector<char> &buf) {
	bool isBinary  = find_str_in_buf("Binary", buf);
	bool versionOK = find_str_in_buf("|KIND=Protel_Advanced_PCB", buf);
	return versionOK && !isBinary;
}

void ADFile::outline_order_segments(std::vector<BRDPoint> &format) {
	std::vector<BRDPoint> source = format;
	std::vector<BRDPoint> ordered;

	ordered.push_back(source.at(0));
	ordered.push_back(source.at(1));
	source.erase(source.begin() + 1);
	source.erase(source.begin());

	auto p   = ordered.back();
	bool hit = false;

	while (source.size() > 1) {

		p   = ordered.back();
		hit = false;

		for (size_t i = 0; i < source.size(); i += 2) {
			auto q = source.at(i);
			auto r = source.at(i + 1);

			if (q.x == p.x && q.y == p.y) {
				ordered.push_back(r);
				source.erase(source.begin() + i + 1);
				source.erase(source.begin() + i);
				hit = true;
				break;
			}
			if (r.x == p.x && r.y == p.y) {
				ordered.push_back(q);
				source.erase(source.begin() + i + 1);
				source.erase(source.begin() + i);
				hit = true;
				break;
			} // if we have a match
		}     // for each possible segment in the list

		if (hit == false) {
			if (source.size() > 1) {
				ordered.push_back(source.at(0));
				ordered.push_back(source.at(1));
				source.erase(source.begin() + 1);
				source.erase(source.begin());
			}
		}
	} // while there's at least 2  items left

	ordered.push_back(ordered.at(0)); // close the loop

	if (ordered.size() > 2) {
		format.clear();
		format = ordered;
	}
}

ADFile::ADFile(std::vector<char> &buf) {
	auto buffer_size = buf.size();

	char *saved_locale;
	saved_locale = setlocale(LC_NUMERIC, "C"); // Use '.' as delimiter for strtod

	ENSURE_OR_FAIL(buffer_size > 4, error_msg, return);
	size_t file_buf_size = 3 * (1 + buffer_size);
	file_buf             = (char *)calloc(1, file_buf_size);
	ENSURE_OR_FAIL(file_buf != nullptr, error_msg, return);

	arena     = &file_buf[buffer_size + 1];
	arena_end = file_buf + file_buf_size - 1;

	std::copy(buf.begin(), buf.end(), file_buf);
	file_buf[buffer_size] = 0;
	*arena_end            = 0;

	int current_block = 0;
	int net_count     = 0;
	BRDPoint format_first, format_last;

	std::vector<char *> lines;
	stringfile(file_buf, lines);

	std::vector<char *>::iterator line_it = lines.begin();
	while (line_it != lines.end()) {
		char *line = *line_it;
		char *p;
		++line_it;

		while (isspace((uint8_t)*line)) line++;
		if (!line[0]) continue;

		if (strstr(line, "RECORD=Track")) {
			current_block = ADFILE_BLOCK_TRACKS;
		}

		if (strstr(line, "RECORD=Net")) {
			current_block = ADFILE_BLOCK_NETS;
		}

		if (strstr(line, "RECORD=Component")) {
			current_block = ADFILE_BLOCK_COMPONENTS;
		}

		if (strstr(line, "RECORD=Pad")) {
			current_block = ADFILE_BLOCK_PADS;
		}

		p = line;

		switch (current_block) {
			case ADFILE_BLOCK_TRACKS: {
				unsigned int part_id;
				int x1, y1, x2, y2;
				char *layer;

				p = strstr(line, "|LAYER=");
				if (p) {
					p += 7;
					layer = read_item(p);
				}

				p = strstr(line, "|COMPONENT=");
				if (p) {
					p += sizeof("|COMPONENT=") - 1;
					part_id = READ_UINT();
					part_id++;
				}

				p = strstr(line, "X1=");
				if (p) {
					p += 3;
					x1 = READ_DOUBLE();
					p  = strstr(p, "Y1=");
					if (p) {
						p += 3;
						y1 = READ_DOUBLE();
						p  = strstr(p, "X2=");
						if (p) {
							p += 3;
							x2 = READ_DOUBLE();
							p  = strstr(p, "Y2=");
							if (p) {
								p += 3;
								y2 = READ_DOUBLE();

								if ((strcmp(layer, "KEEPOUT") == 0)) {
									// Keepout
									//
									// usually the board outline is kept here... usually
									//
									if (format.size() == 0) format_first = BRDPoint({x1, y1});

									format.push_back(BRDPoint({x1, y1}));
									format.push_back(BRDPoint({x2, y2}));
									format_last = BRDPoint({x2, y2});

								} else if (strstr(layer, "OVERLAY")) {
									// Overlay
									//
									for (auto &a_part : ad_parts) {
										if (a_part.part_id == part_id) {
											break;
										}
									}
								} else if ((strncmp(layer, "MECHANICAL", 10) == 0)) {
									// Mechanical
									//
								} else {
									// Failsafe/default
									//
									break;
								}
							} // if Y2= present
						}     // if X2= present
					}         // if Y1= present
				}             // if X1= present
			}                 // ADFILE_BLOCK_TRACKS
				current_block = ADFILE_BLOCK_NONE;
				break;

			case ADFILE_BLOCK_OUTLINE: {
				BRDPoint point;
				if (!strstr(p, "LAYER=KEEPOUT")) break;
				p = strstr(p, "X1=");
				if (p) {
					p += 3;
					point.x = READ_DOUBLE();
					p       = strstr(p, "Y1=");
					if (p) {
						p += 3;
						point.y = READ_DOUBLE();
						format.push_back(point);
						p = strstr(p, "X2=");
						if (p) {
							p += 3;
							point.x = READ_DOUBLE();
							p       = strstr(p, "Y2=");
							if (p) {
								p += 3;
								point.y = READ_DOUBLE();
								format.push_back(point);
							}
						}
					}
				}
			} // ADFILE_BLOCK_OUTLINE
			break;

			case ADFILE_BLOCK_NETS: {
				AD_BRDNet net;
				p = strstr(p, "|ID=");
				if (p) p += 4;
				net.id = READ_INT();
				net.id++;
				net_count++;
				p = strstr(p, "|NAME=");
				if (p) p += 6;
				net.name = read_item(p);
				ad_nets.push_back(net);
				current_block = ADFILE_BLOCK_NONE;

			} // ADFILE_BLOCK_NETS
			break;

			case ADFILE_BLOCK_COMPONENTS: {
				AD_BRDPart part;
				p = strstr(p, "|ID=");
				if (p) {
					p += 4;
					part.part_id = READ_INT();
				} else {
					current_block = ADFILE_BLOCK_NONE;
					break;
				}
				part.part_id++;
				p = strstr(p, "|LAYER=");
				if (p) p += 7;
				part.layer = read_item(p);
				p          = strstr(p, "|X=");
				if (p) p += 3;
				part.x = READ_DOUBLE();
				p      = strstr(p, "|Y=");
				if (p) p += 3;
				part.y = READ_DOUBLE();
				p      = strstr(p, "|ROTATION=");
				if (p) p += 10;
				part.orientation = READ_DOUBLE();
				p                = strstr(p, "|SOURCEDESIGNATOR=");
				if (p) {
					p += 18;
					part.name = read_item(p);
				} else {
					char tn[1024];
					snprintf(tn, sizeof(tn), "UNKNOWN-%d", part.part_id);
					part.name = strdup(tn); // going to lose memory here // MEMORYLEAK
				}
				if (p) p = strstr(p, "|SOURCEDESCRIPTION=");
				if (p) {
					char *t;
					p += 19;
					t                = read_item(p);
					part.description = t;
				}

				ad_parts.push_back(part);
				current_block = ADFILE_BLOCK_NONE;

			} // ADFILE_BLOCK_COMPONENTS
			break;

			case ADFILE_BLOCK_PADS: {
				AD_BRDPad pad;
				bool got_net = false;

				p = strstr(line, "|NET=");
				if (p) {
					p += 5;
					pad.net_id = READ_INT();
					pad.net_id++;
					got_net = true;
				}

				p = strstr(line, "|NAME=");
				if (p) {
					p += 6;
					pad.snum = read_item(p);
					*p       = '|';
				}

				p = strstr(line, "|COMPONENT=");
				if (p) {
					p += 11;
					pad.part_id = READ_INT();
					pad.part_id++;
					if (!got_net) pad.net_id = 0;
				} else {
					current_block = ADFILE_BLOCK_NONE;
					break;
				}

				p = strstr(line, "|X=");
				if (p) {
					p += 3;
					pad.x = READ_DOUBLE();
				} else {
					current_block = ADFILE_BLOCK_NONE;
					break;
				}

				p = strstr(line, "|Y=");
				if (p) {
					p += 3;
					pad.y = READ_DOUBLE();
				} else {
					current_block = ADFILE_BLOCK_NONE;
					break;
				}

				p = strstr(line, "|ROTATION=");
				if (p) {
					char *q;
					p += sizeof("|ROTATION=") - 1;
					q            = strchr(p, '|');
					*q           = '\0';
					pad.rotation = strtod(p, &p);
					*q           = '|';
				}

				p = strstr(line, "|XSIZE=");
				if (p) {
					p += 7;
					pad.x_size = READ_DOUBLE();
				}

				p = strstr(line, "|YSIZE=");
				if (p) {
					p += 7;
					pad.y_size = READ_DOUBLE();
				}

				p = strstr(line, "|INDEXFORSAVE=");
				if (p) {
					p += sizeof("|INDEXFORSAVE=") - 1;
					pad.id = READ_INT();
				}

				p = strstr(line, "|UNIQUEID=");
				if (p) {
					p += sizeof("|UNIQUEID=") - 1;
					pad.unique_id = read_item(p);
					*p            = '|';
				}

				p = strstr(line, "|LAYER=");
				if (p) {
					p += sizeof("|LAYER=") - 1;
					pad.layer = read_item(p);
					if (strcmp(pad.layer, "MULTILAYER") == 0) {
						pad.type = 1;
					}
				}

				if (pad.x_size > 0.0 && pad.y_size > 0.0) {
					pad.radius = (pad.x_size < pad.y_size) ? pad.x_size : pad.y_size;
					pad.radius /= 2;
				}

				ad_pads.push_back(pad);
				current_block = ADFILE_BLOCK_NONE;

			} // ADFILE_BLOCK_PADS
			break;

			default: continue;
		} // switch (current block)
	}     // while more lines to process

	// Altium doesn't include a NC net. so append one to the netlist.
	//
	AD_BRDNet net;
	net.id = net_count;
	net.id++;
	net.name = "NC";
	ad_nets.push_back(net);

	for (auto &ad_part : ad_parts) {
		BRDPart part;

		if (strlen(ad_part.name) < 1) {
			ad_part.name = "UNKNOWN";
		}
		part.name = ad_part.name;

		if (!strcmp(ad_part.layer, "TOP")) {
			part.mounting_side = BRDPartMountingSide::Top;
		} else if (!strcmp(ad_part.layer, "BOTTOM")) {
			part.mounting_side = BRDPartMountingSide::Bottom;
		} else {
			part.mounting_side = BRDPartMountingSide::Both;
		}

		for (auto ad_pad : ad_pads) {
			BRDPin pin;

			if (ad_pad.net_id == 0) {
				ad_pad.net_id = ad_nets.size(); // this pad wasn't assigned a net. assign it to NC net.
			}

			if (ad_part.part_id == ad_pad.part_id) {
				pin.part  = ad_part.part_id;
				pin.pos.x = ad_pad.x;
				pin.pos.y = ad_pad.y;

				for (auto ad_net : ad_nets) {
					if (ad_pad.net_id == ad_net.id) {
						pin.net = ad_net.name;
						break;
					}
				}

				pin.snum   = ad_pad.snum;
				pin.radius = ad_pad.radius;
				if (ad_pad.type == 1) {
					part.part_type = BRDPartType::ThroughHole;
				}
				switch (part.mounting_side) {
					case BRDPartMountingSide::Top:    pin.side = BRDPinSide::Top;    break;
					case BRDPartMountingSide::Bottom: pin.side = BRDPinSide::Bottom; break;
					case BRDPartMountingSide::Both:   pin.side = BRDPinSide::Both;   break;
				}

				pins.push_back(pin);
			} // if part ID and pad.part ID are the same
		}
		part.end_of_pins = pins.size();
		parts.push_back(part);
	}

	std::sort(pins.begin(), pins.end(), customLess);

	// AD files use segments for board outline
	// we want points.
	//
	outline_order_segments(format);

	num_parts  = parts.size();
	num_pins   = pins.size();
	num_format = format.size();
	num_nails  = nails.size();

	setlocale(LC_NUMERIC, saved_locale); // Restore locale

	valid = 1;
}
