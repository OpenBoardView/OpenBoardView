#include "TVWFile.h"

#include "tebo/TeboBoard.hpp"

#include <algorithm>
#include <cstdint>
#include <sstream>

// Every .tvw begins with a length-prefixed, char-ciphered type string that
// decodes to "Tebo-ictview files." — the raw bytes are a stable signature.
bool TVWFile::verifyFormat(std::vector<char> &buf) {
	static const char sig[] = {0x13, 'O', '9', '5', 'w', '-', '2', '8', 'p', 's',
	                           '4', '9', 'm', ' ', '0', '2', 'v', '9', 'o', '.'};
	if (buf.size() < sizeof(sig)) return false;
	return std::equal(std::begin(sig), std::end(sig), buf.begin());
}

const char *TVWFile::intern(const std::string &s) {
	pool_.push_back(s);
	return pool_.back().c_str();
}

namespace {
// Tebo coordinates are 2-decimal fixed point; convert to integer mil like the
// reference Toptest exporter (Fixed32 -> int via float, i.e. raw/100).
inline BRDPoint ToPoint(Tebo::Vector2S v) {
	return BRDPoint(static_cast<int>(static_cast<float>(v.X)),
	                static_cast<int>(static_cast<float>(v.Y)));
}
} // namespace

TVWFile::TVWFile(std::vector<char> &buf) {
	try {
		std::string data(buf.begin(), buf.end());
		std::istringstream is(data, std::ios::in | std::ios::binary);

		Tebo::Board board;
		board.Read(is);

		// Locate the top and bottom copper (logic) layers; parts/pins live there.
		int topIdx = -1, botIdx = -1;
		for (size_t i = 0; i < board.Layers.size(); i++) {
			auto *lo = dynamic_cast<Tebo::LogicLayer *>(board.Layers[i].get());
			if (!lo) continue;
			if (lo->Type == Tebo::LayerType::Top) topIdx = static_cast<int>(i);
			else if (lo->Type == Tebo::LayerType::Bottom) botIdx = static_cast<int>(i);
		}

		// Real board outline: the route ("Roul"/OUTLINE) layer stores the board
		// edge as milled slots. Collect them now, sanity-filter against the pin
		// extents once those are known (some files contain stray segments), and
		// fall back to a bounding box if nothing usable survives.
		std::vector<std::pair<BRDPoint, BRDPoint>> rawOutline;
		for (auto const &lp : board.Layers) {
			auto *tl = dynamic_cast<Tebo::ThroughLayer *>(lp.get());
			if (!tl || tl->Type != Tebo::LayerType::Roul) continue;
			for (auto const &slot : tl->DrillSlots)
				rawOutline.emplace_back(ToPoint(slot.Begin), ToPoint(slot.End));
		}

		// Intern net names once so pins can share the pointers.
		std::vector<const char *> netPtr(board.Nets.size(), nullptr);
		for (size_t i = 0; i < board.Nets.size(); i++)
			netPtr[i] = intern(board.Nets[i]);

		auto layerOf = [&](uint32_t partLayer) -> Tebo::LogicLayer * {
			int idx = -1;
			if (static_cast<int>(partLayer) == topIdx) idx = topIdx;
			else if (static_cast<int>(partLayer) == botIdx) idx = botIdx;
			if (idx < 0) return nullptr;
			return dynamic_cast<Tebo::LogicLayer *>(board.Layers[idx].get());
		};

		BRDPoint bmin{INT32_MAX, INT32_MAX}, bmax{INT32_MIN, INT32_MIN};

		for (Tebo::Part const &part : board.Parts) {
			Tebo::LogicLayer *layer = layerOf(part.Layer);
			if (!layer) continue; // not on top/bottom copper
			bool top = static_cast<int>(part.Layer) == topIdx;

			BRDPart bp;
			bp.name          = intern(part.Name);
			bp.part_type     = BRDPartType::SMD; // refined below if a pad has a hole
			bp.mounting_side = top ? BRDPartMountingSide::Top : BRDPartMountingSide::Bottom;
			bp.p1            = ToPoint(part.Bbox.Min);
			bp.p2            = ToPoint(part.Bbox.Max);
			// Component value / manufacturer part number, e.g. "15P_50V_J_NPO_0201" or
			// "MC74VHC1G32DFT2G_SC70-5". OBV shows this next to the part and searches it.
			if (!part.Value.empty())
				bp.mfgcode = part.Value;
			else if (!part.Desc.empty())
				bp.mfgcode = part.Desc;

			unsigned int const partIndex = static_cast<unsigned int>(parts.size()) + 1;
			for (Tebo::Pin const &pin : part.Pins) {
				BRDPin bpin;
				bpin.part = partIndex;
				bpin.side = top ? BRDPinSide::Top : BRDPinSide::Bottom;
				bpin.snum = intern(pin.Name);
				bpin.name = bpin.snum;

				uint32_t const padIdx = pin.Handle / 8;
				if (padIdx < layer->Pads.size()) {
					Tebo::Pad const &pad = layer->Pads[padIdx];
					bpin.pos = ToPoint(pad.Pos);
					if (pad.Net >= 0 && static_cast<size_t>(pad.Net) < netPtr.size())
						bpin.net = netPtr[pad.Net];
					else
						bpin.net = "UNCONNECTED";
					// through-hole pads mark the whole part as TH
					if (pad.HasHole) bp.part_type = BRDPartType::ThroughHole;
					// draw the pad at its real size instead of a default dot
					if (pad.Shape) {
						float const sx = static_cast<float>(pad.Shape->Size.X);
						float const sy = static_cast<float>(pad.Shape->Size.Y);
						float const d  = (sx > 0.0f && sy > 0.0f) ? std::min(sx, sy) : std::max(sx, sy);
						if (d > 0.0f) bpin.radius = d / 2.0;
					}
				} else {
					bpin.pos = ToPoint(part.Pos);
					bpin.net = "UNCONNECTED";
				}
				bmin.x = std::min(bmin.x, bpin.pos.x);
				bmin.y = std::min(bmin.y, bpin.pos.y);
				bmax.x = std::max(bmax.x, bpin.pos.x);
				bmax.y = std::max(bmax.y, bpin.pos.y);
				pins.push_back(bpin);
			}
			bp.end_of_pins = static_cast<unsigned int>(pins.size());
			parts.push_back(bp);
		}

		// Keep only outline segments that sit near the populated board area: a
		// few files carry stray slots whose coordinates are wildly out of range
		// and would blow the board extents up to thousands of inches.
		if (!rawOutline.empty() && bmin.x <= bmax.x) {
			int const mx = std::max(2000, (bmax.x - bmin.x) / 2);
			int const my = std::max(2000, (bmax.y - bmin.y) / 2);
			auto inRange = [&](const BRDPoint &p) {
				return p.x >= bmin.x - mx && p.x <= bmax.x + mx && p.y >= bmin.y - my && p.y <= bmax.y + my;
			};
			for (auto const &s : rawOutline) {
				if (inRange(s.first) && inRange(s.second)) outline_segments.push_back(s);
			}
		}

		// Fallback outline: rectangle around all pins, only when the file had no
		// route layer to give us the real board edge.
		if (outline_segments.size() < 3 && bmin.x <= bmax.x) {
			format.push_back({bmin.x, bmin.y});
			format.push_back({bmax.x, bmin.y});
			format.push_back({bmax.x, bmax.y});
			format.push_back({bmin.x, bmax.y});
			format.push_back({bmin.x, bmin.y});
		}

		num_parts  = static_cast<unsigned int>(parts.size());
		num_pins   = static_cast<unsigned int>(pins.size());
		num_nails  = static_cast<unsigned int>(nails.size());
		num_format = static_cast<unsigned int>(format.size());
		valid      = num_parts > 0;
		if (!valid) error_msg = "TVW: no parts found on top/bottom layers";
	} catch (const std::exception &e) {
		valid     = false;
		error_msg = std::string("TVW parse error: ") + e.what();
	}
}
