#pragma once
#include "BRDFileBase.h"

struct AD_BRDNet {
	unsigned int id;
	const char *name;
};

struct AD_BRDPart {
	const char *name;
	const char *description;
	const char *layer;

	unsigned int part_id;
	double x;
	double y;
	double orientation;
};

struct AD_BRDPad {
	int id;
	unsigned int net_id;
	unsigned int part_id;
	const char *snum = nullptr;
	double x;
	double y;
	double drill  = 0.0;
	double radius = 0.0;
	double x_size, y_size;
	double rotation = 0.0;
	int type; // SMD = 0, TH = 1
	const char *unique_id;
	const char *layer;
};

struct ADFile : public BRDFileBase {
	ADFile(std::vector<char> &buf);

	struct {
		bool operator()(BRDPin a, BRDPin b) const {
			char *pEnd;
			return strtod(a.snum, &pEnd) < strtod(b.snum, &pEnd);
		}
	} customLess;

	std::vector<AD_BRDNet> ad_nets;
	std::vector<AD_BRDPart> ad_parts;
	std::vector<AD_BRDPad> ad_pads;

	static bool verifyFormat(std::vector<char> &buf);
	void outline_order_segments(std::vector<BRDPoint> &format);
};
