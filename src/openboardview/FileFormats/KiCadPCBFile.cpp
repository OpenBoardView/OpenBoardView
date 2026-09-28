#include "KiCadPCBFile.h"

#include "SExpr.h"

#include <algorithm>
#include <cctype>
#include <clocale>
#include <cmath>
#include <cstring>
#include <unordered_map>

#include <SDL.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const double kMmToMil = 1000.0 / 25.4;
static const char *const kUnconnected = "UNCONNECTED";

bool KiCadPCBFile::verifyFormat(const std::vector<char> &buf) {
	static const char sig[] = "(kicad_pcb";
	static const size_t siglen = sizeof(sig) - 1;
	size_t i = 0;
	while (i < buf.size() && isspace(static_cast<unsigned char>(buf[i]))) ++i;
	return buf.size() - i >= siglen && memcmp(buf.data() + i, sig, siglen) == 0;
}

const char *KiCadPCBFile::intern(const std::string &s) {
	pool_.push_back(s);
	return pool_.back().c_str();
}

// KiCad: mm, Y grows downwards. OBV: integer mil, Y grows upwards.
BRDPoint KiCadPCBFile::to_brd(double mm_x, double mm_y) {
	return BRDPoint(static_cast<int>(std::lround(mm_x * kMmToMil)), static_cast<int>(std::lround(-mm_y * kMmToMil)));
}

// Pad positions are stored footprint-local. The rotation is applied in KiCad's own Y-down frame,
// and back-side footprints need no mirroring: their local coordinates are already flipped in the
// file. Both facts were established against KiCad's GenCAD export, not assumed.
void KiCadPCBFile::Placement::apply(double px, double py, double &wx, double &wy) const {
	wx = x + px * cs + py * sn;
	wy = y - px * sn + py * cs;
}

KiCadPCBFile::KiCadPCBFile(std::vector<char> &buf) {
	char *saved_locale = setlocale(LC_NUMERIC, "C"); // strtod must see '.' as the decimal point

	SExprParser parser(buf.data(), buf.data() + buf.size());
	// Bulky subtrees we never look at. Zone fills alone can be tens of megabytes.
	for (const char *s : {"zone", "segment", "via", "arc", "gr_text", "gr_text_box", "dimension", "target", "group",
	                      "image", "model", "setup", "net_class", "title_block", "paper", "general", "embedded_fonts",
	                      "embedded_files", "generator", "generator_version", "host"}) {
		parser.skip(s);
	}

	SExpr root;
	if (!parser.parse(root)) {
		error_msg = "KiCad PCB: parse error: " + parser.error();
		setlocale(LC_NUMERIC, saved_locale);
		return;
	}
	if (root.head() != "kicad_pcb") {
		error_msg = "KiCad PCB: not a kicad_pcb file (found '" + root.head() + "')";
		setlocale(LC_NUMERIC, saved_locale);
		return;
	}

	// Net table: (net <id> "<name>"). Id 0 is KiCad's "no net".
	std::vector<const char *> net_names;
	for (const SExpr *n : root.find_all("net")) {
		long id = static_cast<long>(n->num(1, -1));
		if (id < 0) continue;
		if (static_cast<size_t>(id) >= net_names.size()) net_names.resize(id + 1, kUnconnected);
		const std::string &name = n->str(2);
		net_names[id]           = (id == 0 || name.empty()) ? kUnconnected : intern(name);
	}

	// Footprints: (footprint ...) since KiCad 6, (module ...) before that.
	for (const SExpr &c : root.list) {
		if (!c.is_list) continue;
		if (c.head() == "footprint" || c.head() == "module") parse_footprint(c, net_names);
	}

	// Board outline drawn at board level. Footprint-level Edge.Cuts are handled in parse_footprint.
	for (const SExpr &c : root.list) {
		if (!c.is_list) continue;
		const std::string &h = c.head();
		if (h == "gr_line" || h == "gr_rect" || h == "gr_circle" || h == "gr_arc" || h == "gr_poly") {
			parse_edge_item(c, nullptr);
		}
	}
	if (outline_segments.size() < 3) {
		outline_segments.clear();
		fallback_outline();
	}

	num_parts  = parts.size();
	num_pins   = pins.size();
	num_nails  = nails.size();
	num_format = format.size();
	valid      = num_parts > 0;
	if (!valid) error_msg = "KiCad PCB: no footprints with pads found";

	setlocale(LC_NUMERIC, saved_locale);
}

void KiCadPCBFile::parse_footprint(const SExpr &fp, const std::vector<const char *> &net_names) {
	Placement place;
	if (const SExpr *at = fp.find("at")) {
		place.x    = at->num(1);
		place.y    = at->num(2);
		double rot = at->num(3) * M_PI / 180.0;
		place.cs   = cos(rot);
		place.sn   = sin(rot);
	}
	if (const SExpr *layer = fp.find("layer")) place.back = (layer->str(1) == "B.Cu");

	// Reference/value: (property "Reference" "R1" ...) from KiCad 7; (fp_text reference "R1" ...) before.
	std::string reference, value;
	for (const SExpr *p : fp.find_all("property")) {
		if (p->str(1) == "Reference") reference = p->str(2);
		else if (p->str(1) == "Value") value = p->str(2);
	}
	for (const SExpr *t : fp.find_all("fp_text")) {
		if (t->str(1) == "reference" && reference.empty()) reference = t->str(2);
		else if (t->str(1) == "value" && value.empty()) value = t->str(2);
	}

	std::vector<const SExpr *> pads = fp.find_all("pad");
	if (pads.empty()) return; // logos, mounting holes without pads, etc.

	BRDPart part;
	part.name          = intern(reference.empty() ? std::string("?") : reference);
	part.mfgcode       = value;
	part.part_type     = BRDPartType::SMD;
	part.mounting_side = place.back ? BRDPartMountingSide::Bottom : BRDPartMountingSide::Top;
	unsigned int const part_index = static_cast<unsigned int>(parts.size()) + 1;

	int minx = 0, miny = 0, maxx = 0, maxy = 0;
	bool first = true;
	for (const SExpr *pad : pads) {
		BRDPin pin;
		pin.part = part_index;
		pin.snum = intern(pad->str(1));
		pin.name = pin.snum;

		const std::string &type = pad->str(2); // smd | thru_hole | np_thru_hole | connect
		if (type == "thru_hole") part.part_type = BRDPartType::ThroughHole;

		double px = 0, py = 0;
		if (const SExpr *at = pad->find("at")) {
			px = at->num(1);
			py = at->num(2);
		}
		double wx, wy;
		place.apply(px, py, wx, wy);
		pin.pos = to_brd(wx, wy);

		if (const SExpr *size = pad->find("size")) {
			double w = size->num(1), h = size->num(2, size->num(1));
			double d = (w > 0 && h > 0) ? std::min(w, h) : std::max(w, h);
			if (d > 0) pin.radius = d * kMmToMil / 2.0;
		}

		// Which copper the pad is on decides which side it is visible from.
		bool on_front = false, on_back = false;
		if (const SExpr *layers = pad->find("layers")) {
			for (size_t i = 1; i < layers->size(); i++) {
				const std::string &l = layers->str(i);
				if (l == "*.Cu") on_front = on_back = true;
				else if (l == "F.Cu") on_front = true;
				else if (l == "B.Cu") on_back = true;
			}
		}
		if (on_front && on_back) pin.side = BRDPinSide::Both;
		else if (on_front) pin.side = BRDPinSide::Top;
		else if (on_back) pin.side = BRDPinSide::Bottom;
		else pin.side = place.back ? BRDPinSide::Bottom : BRDPinSide::Top;

		pin.net = kUnconnected;
		if (const SExpr *net = pad->find("net")) {
			long id = static_cast<long>(net->num(1, -1));
			if (id > 0 && static_cast<size_t>(id) < net_names.size()) pin.net = net_names[id];
		}

		if (first) {
			minx = maxx = pin.pos.x;
			miny = maxy = pin.pos.y;
			first = false;
		} else {
			minx = std::min(minx, pin.pos.x);
			maxx = std::max(maxx, pin.pos.x);
			miny = std::min(miny, pin.pos.y);
			maxy = std::max(maxy, pin.pos.y);
		}
		pins.push_back(pin);
	}
	part.p1          = BRDPoint(minx, miny);
	part.p2          = BRDPoint(maxx, maxy);
	part.end_of_pins = static_cast<unsigned int>(pins.size());
	parts.push_back(part);

	// Some designs draw part of the board edge inside a footprint (e.g. a connector cutout).
	for (const SExpr &c : fp.list) {
		if (!c.is_list) continue;
		const std::string &h = c.head();
		if (h == "fp_line" || h == "fp_rect" || h == "fp_circle" || h == "fp_arc" || h == "fp_poly") {
			parse_edge_item(c, &place);
		}
	}
}

void KiCadPCBFile::parse_edge_item(const SExpr &item, const Placement *place) {
	const SExpr *layer = item.find("layer");
	if (!layer || layer->str(1) != "Edge.Cuts") return;

	const std::string &h = item.head();
	const SExpr *start   = item.find("start");
	const SExpr *end     = item.find("end");
	if (h == "gr_line" || h == "fp_line") {
		if (start && end) add_segment(start->num(1), start->num(2), end->num(1), end->num(2), place);
	} else if (h == "gr_rect" || h == "fp_rect") {
		if (start && end) {
			double x1 = start->num(1), y1 = start->num(2), x2 = end->num(1), y2 = end->num(2);
			add_segment(x1, y1, x2, y1, place);
			add_segment(x2, y1, x2, y2, place);
			add_segment(x2, y2, x1, y2, place);
			add_segment(x1, y2, x1, y1, place);
		}
	} else if (h == "gr_circle" || h == "fp_circle") {
		const SExpr *center = item.find("center");
		if (center && end) {
			double cx = center->num(1), cy = center->num(2);
			add_circle(cx, cy, hypot(end->num(1) - cx, end->num(2) - cy), place);
		}
	} else if (h == "gr_arc" || h == "fp_arc") {
		const SExpr *mid = item.find("mid");
		if (start && mid && end) {
			add_arc(start->num(1), start->num(2), mid->num(1), mid->num(2), end->num(1), end->num(2), place);
		}
	} else if (h == "gr_poly" || h == "fp_poly") {
		const SExpr *pts = item.find("pts");
		if (!pts) return;
		std::vector<const SExpr *> xy = pts->find_all("xy");
		for (size_t i = 0; i + 1 <= xy.size() && xy.size() >= 2; i++) {
			const SExpr *a = xy[i], *b = xy[(i + 1) % xy.size()];
			add_segment(a->num(1), a->num(2), b->num(1), b->num(2), place);
		}
	}
}

void KiCadPCBFile::add_segment(double x1, double y1, double x2, double y2, const Placement *place) {
	if (place) {
		double tx, ty;
		place->apply(x1, y1, tx, ty);
		x1 = tx;
		y1 = ty;
		place->apply(x2, y2, tx, ty);
		x2 = tx;
		y2 = ty;
	}
	outline_segments.push_back(std::make_pair(to_brd(x1, y1), to_brd(x2, y2)));
}

void KiCadPCBFile::add_circle(double cx, double cy, double r, const Placement *place) {
	const int n = 32;
	double px = cx + r, py = cy;
	for (int i = 1; i <= n; i++) {
		double a  = 2.0 * M_PI * i / n;
		double qx = cx + r * cos(a), qy = cy + r * sin(a);
		add_segment(px, py, qx, qy, place);
		px = qx;
		py = qy;
	}
}

// KiCad arcs are given as three points on the arc: start, mid, end.
void KiCadPCBFile::add_arc(double sx, double sy, double mx, double my, double ex, double ey, const Placement *place) {
	// Circumcentre of the three points.
	double d = 2.0 * (sx * (my - ey) + mx * (ey - sy) + ex * (sy - my));
	if (fabs(d) < 1e-9) { // collinear: degrade to a straight line
		add_segment(sx, sy, ex, ey, place);
		return;
	}
	double s2 = sx * sx + sy * sy, m2 = mx * mx + my * my, e2 = ex * ex + ey * ey;
	double cx = (s2 * (my - ey) + m2 * (ey - sy) + e2 * (sy - my)) / d;
	double cy = (s2 * (ex - mx) + m2 * (sx - ex) + e2 * (mx - sx)) / d;
	double r  = hypot(sx - cx, sy - cy);

	double a0 = atan2(sy - cy, sx - cx);
	double am = atan2(my - cy, mx - cx);
	double a1 = atan2(ey - cy, ex - cx);
	// Sweep from a0 to a1 in the direction that passes through am.
	double sweep = a1 - a0;
	double tomid = am - a0;
	while (sweep <= -M_PI) sweep += 2 * M_PI;
	while (sweep > M_PI) sweep -= 2 * M_PI;
	while (tomid <= -M_PI) tomid += 2 * M_PI;
	while (tomid > M_PI) tomid -= 2 * M_PI;
	if ((sweep >= 0) != (tomid >= 0) || fabs(tomid) > fabs(sweep)) {
		sweep = sweep >= 0 ? sweep - 2 * M_PI : sweep + 2 * M_PI;
	}

	int n = std::max(4, static_cast<int>(fabs(sweep) / (2 * M_PI) * 32 + 0.5));
	double px = sx, py = sy;
	for (int i = 1; i <= n; i++) {
		double a  = a0 + sweep * i / n;
		double qx = cx + r * cos(a), qy = cy + r * sin(a);
		add_segment(px, py, qx, qy, place);
		px = qx;
		py = qy;
	}
}

// No usable Edge.Cuts: draw a box around the pins so the board still renders.
void KiCadPCBFile::fallback_outline() {
	if (pins.empty()) return;
	int minx = pins[0].pos.x, maxx = minx, miny = pins[0].pos.y, maxy = miny;
	for (const BRDPin &p : pins) {
		minx = std::min(minx, p.pos.x);
		maxx = std::max(maxx, p.pos.x);
		miny = std::min(miny, p.pos.y);
		maxy = std::max(maxy, p.pos.y);
	}
	const int margin = 100; // mil
	format.push_back(BRDPoint(minx - margin, miny - margin));
	format.push_back(BRDPoint(maxx + margin, miny - margin));
	format.push_back(BRDPoint(maxx + margin, maxy + margin));
	format.push_back(BRDPoint(minx - margin, maxy + margin));
	format.push_back(BRDPoint(minx - margin, miny - margin));
}
