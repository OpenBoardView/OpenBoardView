#include "BRDFile.h"

#include "BRDBoard.h"
#include "json11\json11.hpp"
#include "platform.h"

#include <vector>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cerrno>
#include <functional>

using namespace std;
using namespace std::placeholders;
using namespace json11;

const string BRDBoard::kNetUnconnectedPrefix = "UNCONNECTED";
const string BRDBoard::kComponentDummyName = "...";

BRDBoard::BRDBoard(const BRDFile* const boardFile) 
	: m_file(boardFile)
{
    // FIXME: just loop through original arrays?
    vector<BRDPart> m_parts(m_file->num_parts);
    vector<BRDPin> m_pins(m_file->num_pins);
    vector<BRDNail> m_nails(m_file->num_nails);
    vector<BRDPoint> m_points(m_file->num_format);

	m_parts.assign(m_file->parts, m_file->parts + m_file->num_parts);
	m_pins.assign(m_file->pins, m_file->pins + m_file->num_pins);
	m_nails.assign(m_file->nails, m_file->nails + m_file->num_nails);
    m_points.assign(m_file->format, m_file->format + m_file->num_format);

    // TODO: strip / trim all strings, especially those used as keys

    // Set outline
    {
        for (auto& brdPoint : m_points) {
            shared_ptr<Point> point = make_shared<Point>(brdPoint.x, brdPoint.y);
            outline_.push_back(point);
        }
    }

    // Populate map of unique nets
    SharedStringMap<Net> net_map;
    {
        // adding special net 'UNCONNECTED'
        auto net_nc = make_shared<Net>();
        net_nc->name = kNetUnconnectedPrefix;
        net_nc->is_ground = false;
        net_map[net_nc->name] = net_nc;

        // handle all the others
        for (auto& brd_nail : m_nails) {
            auto net = make_shared<Net>();

            // copy NET name and number (probe)
            net->name = string(brd_nail.net);
            
            // avoid having multiple UNCONNECTED<XXX> references
            if (is_prefix(kNetUnconnectedPrefix, net->name))
                continue;

            // check whether the pin represents ground
            net->is_ground = (net->name == "GND");
            net->number = brd_nail.probe;   // FIXME: probe info missing

            // NOTE: brd_nail.side not handled here

            // so we can find nets later by name (making unique by name)
            net_map[net->name] = net;
        }
    }

    // Populate parts
    {
        for (auto& brd_part : m_parts) {
            auto comp = make_shared<Component>();

            comp->name = string(brd_part.name);

            // is it some dummy component to indicate test pads?
            if (is_prefix(kComponentDummyName, comp->name))
                comp->component_type = Component::kComponentTypeDummy;

            // check what side the board is on (sorcery?)
            if (brd_part.type < 8 && brd_part.type >= 4) {
                comp->board_side = kBoardSideTop;
            }
            else if (brd_part.type >= 8) {
                comp->board_side = kBoardSideBottom;
            }
            else {
                comp->board_side = kBoardSideBoth; // ???
            }

            comp->mount_type = brd_part.type & 0xc ? // BRD specific
                Component::kMountTypeSMD : Component::kMountTypeDIP;

            components_.push_back(comp);
        }
    }

    // Populate pins
    {
        // generate dummy component as reference
        auto comp_dummy = make_shared<Component>();
        comp_dummy->name = kComponentDummyName;
        comp_dummy->component_type = Component::kComponentTypeDummy;

        // NOTE: originally the pin diameter depended on part.name[0] == 'U' ?
        int pin_idx = 0;
        int part_idx = 1;
        auto pins = m_pins;
        auto parts = m_parts;

        for (int i = 0; i < pins.size(); i++) {
            // (from BoardView::DrawPins)
            const BRDPin &brd_pin = pins[i];
            const BRDPart &brd_part = parts[brd_pin.part - 1];
            Component *comp = components_[brd_pin.part - 1].get();

            auto pin = make_shared<Pin>();

            if (comp->is_dummy()) {
                // component is virtual, i.e. "...", pin is test pad
                pin->type = Pin::kPinTypeTestPad;
                pin->component = comp_dummy.get();
                comp_dummy->pins.push_back(pin.get());
            }
            else {
                // component is regular / not virtual
                pin->type = Pin::kPinTypeComponent;
                pin->component = comp;
                comp->pins.push_back(pin.get());
            }

            // determine pin number on part
            ++pin_idx;
            if (brd_pin.part != part_idx) {
                part_idx = brd_pin.part;
                pin_idx = 1;
            }
            pin->number = pin_idx;

            // copy position
            pin->position = Point(brd_pin.pos.x, brd_pin.pos.y);

            // set net reference (here's our NET key string again)
            string net_name = string(brd_pin.net);
            if (net_map.count(net_name)) {
                // there is a net with that name in our map
                pin->net = net_map[net_name].get();
            }
            else {
                // no net with that name registered, so create one
                if (!net_name.empty()) {
                    if (is_prefix(kNetUnconnectedPrefix, net_name)) {
                        // pin is unconnected, so reference our special net
                        pin->net = net_map[kNetUnconnectedPrefix].get();
                        pin->type = Pin::kPinTypeNotConnected;
                    }
                    else {
                        // indeed a new net
                        auto net = make_shared<Net>();
                        net->name = net_name;
                        net->board_side = pin->board_side;
                        // NOTE: net->number not set
                        net_map[net_name] = net;
                        pin->net = net.get();
                    }
                }
                else {
                    // not sure this can happen -> no info
                    pin->net = nullptr;
                    pin->type = Pin::kPinTypeUnkown;
                }
            }

            pin->net->pins.push_back(pin.get());

            pin->diameter = 0.5f;    // TODO

            pins_.push_back(pin);
        }

        // remove all dummy components from vector, add our official dummy
        components_.erase(
            remove_if(begin(components_), end(components_), 
                [](shared_ptr<Component>& comp)
                {
                    return comp->is_dummy();
                }),
            end(components_));

        components_.push_back(comp_dummy);
    }

    // Populate Net vector by using the map. (sorted by keys)
    for (auto& net : net_map) {
        nets_.push_back(net.second);
    }

    // Sort components by name
    sort(begin(components_), end(components_), 
        [](shared_ptr<Component>& lhs, shared_ptr<Component>& rhs) {
            return lhs->name < rhs->name;
        });
}

BRDBoard::~BRDBoard()
{
}

//TODO: error handling
bool BRDBoard::FetchPartAnnotations() {
    const char* fname = "annotations.json";

    // Read annotation source file.
    ifstream json_file;
    stringstream file_buf;
    json_file.exceptions(ifstream::failbit | ifstream::badbit);
    try {
        json_file.open(fname);
        file_buf << json_file.rdbuf();
        json_file.close();
    }
    catch (ifstream::failure e) {
        return false;
    }

    // Parse JSON.
    string json_str = file_buf.str();
    string error;
    Json json = Json::parse(json_str, error);

    if (error.length() > 0)
        return false;

    map<string,string> annotations;
    for (auto part : json["part_annotations"].array_items()) {
        annotations[part["part"].string_value()] = part["name"].string_value();
    }

    // Set / apply annotations.
    #pragma warning(disable : 4996)
    //for (auto& part : m_parts) {
    //    string part_name(part.name);
    //    if (annotations.count(part_name)) {
    //        //delete[] part.annotation;
    //        //char *buf = new char[annotations[part_name].length()];
    //        //strcpy(buf, annotations[part_name].c_str());
    //        //part.annotation = buf;
    //    }
    //}

    return true;
}

SharedVector<Component> &BRDBoard::Components()
{
    return components_;
}

SharedVector<Pin> &BRDBoard::Pins()
{
    return pins_;
}

SharedVector<Net> &BRDBoard::Nets()
{
    return nets_;
}

SharedVector<Point> &BRDBoard::OutlinePoints()
{
    return outline_;
}

Board::EBoardType BRDBoard::BoardType()
{
	return kBoardTypeBRD;
}

