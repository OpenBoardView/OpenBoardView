#pragma once

#include "BRDFileBase.h"

#include <deque>
#include <string>
#include <vector>

struct SExpr;

// KiCad PCB files (*.kicad_pcb), the S-expression format used since KiCad 4.
//
// Footprints become parts, pads become pins carrying their net name, and whatever sits on the
// Edge.Cuts layer becomes the board outline. Tracks, zones, text and 3D models are skipped.
// Coordinates are converted from mm (Y down) to OBV's mil (Y up).
class KiCadPCBFile : public BRDFileBase {
  public:
	KiCadPCBFile(std::vector<char> &buf);

	static bool verifyFormat(const std::vector<char> &buf);

  private:
	// Placement of one footprint: translation plus rotation, applied to pad-local mm coordinates.
	struct Placement {
		double x = 0, y = 0;   // mm
		double cs = 1, sn = 0; // cos/sin of the footprint rotation
		bool back = false;
		void apply(double px, double py, double &wx, double &wy) const;
	};

	// Stable storage for the const char* names handed to BRDFileBase.
	std::deque<std::string> pool_;
	const char *intern(const std::string &s);

	static BRDPoint to_brd(double mm_x, double mm_y);

	void parse_footprint(const SExpr &fp, const std::vector<const char *> &net_names);
	void parse_edge_item(const SExpr &item, const Placement *place);
	void add_segment(double x1, double y1, double x2, double y2, const Placement *place);
	void add_arc(double sx, double sy, double mx, double my, double ex, double ey, const Placement *place);
	void add_circle(double cx, double cy, double r, const Placement *place);
	void fallback_outline();
};
