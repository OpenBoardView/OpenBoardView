#include "BRDBoard.h"

#include "FileFormats/BRDFile.h"

#include <cerrno>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using namespace std::placeholders;

const string BRDBoard::kNetUnconnectedPrefix = "UNCONNECTED";
const string BRDBoard::kComponentDummyName   = "...";

BRDBoard::BRDBoard(const BRDFile *const boardFile)
    : m_file(boardFile) {
	// TODO: strip / trim all strings, especially those used as keys
	// TODO: just loop through original arrays?
	vector<BRDPart> m_parts(m_file->num_parts);
	vector<BRDPin> m_pins(m_file->num_pins);
	vector<BRDNail> m_nails(m_file->num_nails);
	vector<BRDPoint> m_points(m_file->num_format);

	Component * dummy_comp_ = nullptr;
	
	m_parts  = m_file->parts;
	m_pins   = m_file->pins;
	m_nails  = m_file->nails;
	m_points = m_file->format;

	// Set outline
	{
		for (auto &brdPoint : m_points) {
			auto point = obv_make_shared<Point>(brdPoint.x, brdPoint.y);
			outline_.push_back(point);
		}
	}

	// Populate map of unique nets
	SharedStringMap<Net> net_map;
	{
		// adding special net 'UNCONNECTED'
		auto net_nc           = obv_make_shared<Net>();
		net_nc->name          = kNetUnconnectedPrefix;
		net_nc->is_ground     = false;
		net_map[net_nc->name] = net_nc;

		// handle all the others
		for (auto &brd_nail : m_nails) {
			auto net = obv_make_shared<Net>();

			// copy NET name and number (probe)
			net->name = string(brd_nail.net);

			// avoid having multiple UNCONNECTED<XXX> references
			if (is_prefix(kNetUnconnectedPrefix, net->name)) continue;

			net->number    = brd_nail.probe;

			if (brd_nail.side == 1) {
				net->board_side = kBoardSideTop;
			} else {
				net->board_side = kBoardSideBottom;
			}

			// so we can find nets later by name (making unique by name)
			net_map[net->name] = net;
		}
	}

	// Populate parts
	{
		int make = 0;
		for (auto &brd_part : m_parts) {
			auto comp = obv_make_shared<Component>();
			++make;
			comp->name    = string(brd_part.name);
			comp->mfgcode = std::move(brd_part.mfgcode);

			comp->p1 = {float(brd_part.p1.x), float(brd_part.p1.y)};
			comp->p2 = {float(brd_part.p2.x), float(brd_part.p2.y)};

			// is it some dummy component to indicate test pads?
			if (is_prefix(kComponentDummyName, comp->name)) comp->component_type = Component::kComponentTypeDummy;

			// check what side the board is on (sorcery?)
			if (brd_part.mounting_side == BRDPartMountingSide::Top) {
				comp->board_side = kBoardSideTop;
			} else if (brd_part.mounting_side == BRDPartMountingSide::Bottom) {
				comp->board_side = kBoardSideBottom;
			} else {
				comp->board_side = kBoardSideBoth;
			}
					 
			comp->mount_type = (brd_part.part_type == BRDPartType::SMD) ? Component::kMountTypeSMD : Component::kMountTypeDIP;

			components_.push_back(comp);
		}
	}

	// Populate pins
	{
		// generate dummy component as reference
		obv_shared_ptr<Component> comp_dummy = nullptr;

		if constexpr (! std::is_same<obv_shared_ptr<Component>, std::shared_ptr<Component> >::value) {
			if (! dummy_comp_) {
				dummy_comp_ = new Component();
			}
			comp_dummy            = obv_shared_ptr<Component>(dummy_comp_); //obv_make_shared<Component>(dummy_comp_);
		} else {
			comp_dummy            = obv_make_shared<Component>();
		}
		comp_dummy->name           = kComponentDummyName;
		comp_dummy->component_type = Component::kComponentTypeDummy;

		// NOTE: originally the pin diameter depended on part.name[0] == 'U' ?
		unsigned int pin_idx  = 0;
		unsigned int part_idx = 1;
		auto pins             = m_pins;
		auto parts            = m_parts;

		for (size_t i = 0; i < pins.size(); i++) {
			// (originally from BoardView::DrawPins)
			const BRDPin &brd_pin = pins[i];
			obv_shared_ptr<Component> comp       = components_[brd_pin.part - 1];

			if (!comp) continue;

			auto pin = obv_make_shared<Pin>();

			if (comp->is_dummy()) {
				// component is virtual, i.e. "...", pin is test pad
				pin->type      = Pin::kPinTypeTestPad;
				pin->component = comp_dummy;
				comp_dummy->pins.push_back(pin);
			} else {
				// component is regular / not virtual
				pin->type       = Pin::kPinTypeComponent;
				pin->component  = comp;
				pin->board_side = pin->component->board_side;
				comp->pins.push_back(pin);
			}

			// determine pin number on part
			++pin_idx;
			if (brd_pin.part != part_idx) {
				part_idx = brd_pin.part;
				pin_idx  = 1;
			}
			if (brd_pin.snum) {
				pin->number = brd_pin.snum;
			} else {
				pin->number = std::to_string(pin_idx);
			}

			// Lets us see BGA pad names finally
			//
			if (brd_pin.name) {
				pin->name = std::string(brd_pin.name);
			} else {
				pin->name = pin->number;
			}

			// copy position
			pin->position = Point(brd_pin.pos.x, brd_pin.pos.y);

			// set net reference (here's our NET key string again)
			string net_name = string(brd_pin.net);
			if (net_map.count(net_name)) {
				// there is a net with that name in our map
				pin->net = net_map[net_name].get();

				if (pin->type == Pin::kPinTypeTestPad) {
					pin->board_side = pin->net->board_side;
				}
			} else {
				// no net with that name registered, so create one
				if (!net_name.empty()) {
					if (is_prefix(kNetUnconnectedPrefix, net_name)) {
						// pin is unconnected, so reference our special net
						pin->net  = net_map[kNetUnconnectedPrefix].get();
						pin->type = Pin::kPinTypeNotConnected;
					} else {
						// indeed a new net
						auto net        = obv_make_shared<Net>();
						net->name       = net_name;
						net->board_side = pin->board_side;
						// NOTE: net->number not set
						net_map[net_name] = net;
						pin->net          = net.get();
					}
				} else {
					// not sure this can happen -> no info
					// It does happen in .fz apparently and produces a SEGFAULT… Use
					// unconnected net.
					pin->net  = net_map[kNetUnconnectedPrefix].get();
					pin->type = Pin::kPinTypeNotConnected;
				}
			}

			// TODO: should either depend on file specs or type etc
			//
			//  if(brd_pin.radius) pin->diameter = brd_pin.radius; // some format
			//  (.fz) contains a radius field
			//    else pin->diameter = 0.5f;
			pin->diameter = brd_pin.radius; // some format (.fz) contains a radius field

			pin->net->pins.push_back(pin);
			pins_.push_back(pin);
		}

		// remove all dummy components from vector, add our official dummy

		if constexpr (! std::is_same<obv_shared_ptr<Component>, std::shared_ptr<Component> >::value) {
			components_.erase(
							  remove_if(begin(components_), end(components_), [](obv_shared_ptr<Component> &comp) { if (comp->is_dummy()) { delete comp.get(); return true; } return false; }),
							  end(components_));
		} else {
			components_.erase(
							  remove_if(begin(components_), end(components_), [](obv_shared_ptr<Component> &comp) { return comp->is_dummy(); }),
							  end(components_));
		}
		
		components_.push_back(comp_dummy);
	}

	// Populate Net vector by using the map. (sorted by keys)
	for (auto &net : net_map) {
		// check whether the pin represents ground
		net.second->is_ground = (net.second->name == "GND" || net.second->name == "GROUND");
		nets_.push_back(net.second);
	}

	// Sort components by name
	sort(begin(components_), end(components_), [](const obv_shared_ptr<Component> &lhs, const obv_shared_ptr<Component> &rhs) {
		return lhs->name < rhs->name;
	});
}

BRDBoard::~BRDBoard() {
	if constexpr (! std::is_same<obv_shared_ptr<Component>, std::shared_ptr<Component> >::value) {
		for (auto && ci : components_) {
			delete ci.get();
		}
		for (auto && ni : nets_) {
			delete ni.get();
		}
		for (auto && pi : pins_) {
			delete pi.get();
		}
	}
	if constexpr (Component::do_refcount) {
		std::cerr << "component count after board delete: " << Component::icount << "\n";
	}
}

SharedVector<Node> &BRDBoard::Nodes() {
	return nodes_;
}

SharedVector<Component> &BRDBoard::Components() {
	return components_;
}

SharedVector<Pin> &BRDBoard::Pins() {
	return pins_;
}

SharedVector<Net> &BRDBoard::Nets() {
	return nets_;
}

SharedVector<Point> &BRDBoard::OutlinePoints() {
	return outline_;
}

Board::EBoardType BRDBoard::BoardType() {
	return kBoardTypeBRD;
}
