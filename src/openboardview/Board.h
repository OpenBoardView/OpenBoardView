#pragma once

#include "FileFormats/BRDFile.h"

#include "imgui/imgui.h"
#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#define EMPTY_STRING ""
#define kBoardComponentPrefix "c_"
#define kBoardPinPrefix "p_"
#define kBoardNetPrefix "n_"
#define kBoardElementNameLength 127

struct Point;
struct BoardElement;
struct Net;
struct Pin;
struct Component;

typedef std::function<void(const char *)> TcharStringCallback;
typedef std::function<void(BoardElement *)> TboardElementCallback;

template <class T>
using SharedVector = std::vector<std::shared_ptr<T>>;

template <class T>
using SharedStringMap = std::map<std::string, std::shared_ptr<T>>;

enum EBoardSide {
	kBoardSideTop    = 0,
	kBoardSideBottom = 1,
	kBoardSideBoth   = 2,
};

// Checking whether str `prefix` is a prefix of str `base`.
inline static bool is_prefix(std::string prefix, std::string base) {
	if (prefix.size() > base.size()) return false;

	auto res = mismatch(prefix.begin(), prefix.end(), base.begin());
	if (res.first == prefix.end()) return true;

	return false;
}

template <class T>
inline static bool contains(const T &element, std::vector<T> &v) {
	return find(begin(v), end(v), element) != end(v);
}

template <class T>
inline static bool contains(T &element, std::vector<T> &v) {
	return find(begin(v), end(v), element) != end(v);
}

template <class T>
inline void remove(T &element, std::vector<T> &v) {

	auto it = std::find(v.begin(), v.end(), element);

	if (it != v.end()) {
		using std::swap;
		swap(*it, v.back());
		v.pop_back();
	}
}

// Any element being on the board.
struct BoardElement {
	// Side of the board the element is located. (top, bottom, both?)
	EBoardSide board_side = kBoardSideBoth;

	// String uniquely identifying this element on the board.
	virtual std::string UniqueId() const = 0;
};

// A point/position on the board relative to top left corner of the board.
// TODO: not sure how different formats will store this info.
struct Point {
	float x, y;

	Point()
	    : x(0.0f)
	    , y(0.0f){};
	Point(float _x, float _y)
	    : x(_x)
	    , y(_y){};
	Point(int _x, int _y)
	    : x(float(_x))
	    , y(float(_y)){};
};

// Shared potential between multiple Pins/Contacts.
struct Net : BoardElement {
	int number;
	std::string name;
	bool is_ground;

	SharedVector<Pin> pins;

	std::string UniqueId() const {
		return kBoardNetPrefix + name;
	}

	std::vector<const std::string *> searchableStringDetails() const;
};

// Any observeable contact (nails, component pins).
// Convieniently/Confusingly named Pin not Contact here.
struct Pin : BoardElement {
	enum EPinType {
		kPinTypeUnkown = 0,
		kPinTypeNotConnected,
		kPinTypeComponent,
		kPinTypeVia,
		kPinTypeTestPad,
	};

	// Type of Contact, e.g. pin, via, probe/test point.
	EPinType type;

	// Pin number / Nail count.
	std::string number;

	std::string name; // for BGA pads will be AZ82 etc

	// Position according to board file. (probably in inches)
	Point position;

	// Contact diameter, e.g. via or pin size. (probably in inches)
	float diameter;

	// Net this contact is connected to, nulltpr if no info available.
	Net *net;

	// Pin comment
	std::string comment;

	// Contact belonging to this component (pin), nullptr if nail.
	std::shared_ptr<Component> component;

	std::string UniqueId() const {
		return kBoardPinPrefix + number;
	}
};

// A component on the board having multiple Pins.
struct Component : BoardElement {
	enum EMountType { kMountTypeUnknown = 0, kMountTypeSMD, kMountTypeDIP };

	enum EComponentType {
		kComponentTypeUnknown = 0,
		kComponentTypeDummy,
		kComponentTypeConnector,
		kComponentTypeIC,
		kComponentTypeResistor,
		kComponentTypeCapacitor,
		kComponentTypeDiode,
		kComponentTypeTransistor,
		kComponentTypeCrystal,
		kComponentTypeJellyBean
	};

	// How the part is attached to the board, either SMD, .., through-hole?
	EMountType mount_type = kMountTypeUnknown;

	// Type of component, eg. resistor, cap, etc.
	EComponentType component_type = kComponentTypeUnknown;

	// Part name as stored in board file.
	std::string name;

	// Part manufacturing code (aka. part number).
	std::string mfgcode;

	// Pins belonging to this component.
	SharedVector<Pin> pins;

	// Post calculated outlines
	//
	std::array<ImVec2, 4> outline;
	Point p1{0.0f, 0.0f}, p2{0.0f, 0.0f}; // for debugging

	bool outline_done = false;
	std::vector<ImVec2> hull;
	ImVec2 omin, omax;
	ImVec2 centerpoint;
	double expanse = 0.0f; // quick measure of distance between pins.

	// enum ComponentVisualModes { CVMNormal = 0, CVMSelected, CVMShowPins, CVMModeCount };
	enum ComponentVisualModes { CVMNormal = 0, CVMSelected, CVMModeCount };

	uint8_t visualmode = 0;

	// Mount type as readable string.
	std::string mount_type_str() {
		switch (mount_type) {
			case Component::kMountTypeSMD: return "SMD";
			case Component::kMountTypeDIP: return "DIP";
			default: return "UNKNOWN";
		}
	}

	// true if component is not representing a real/physical component.
	bool is_dummy() {
		return component_type == kComponentTypeDummy;
	}

	std::string UniqueId() const {
		return kBoardComponentPrefix + name;
	}

	std::vector<const std::string *> searchableStringDetails() const;
};

class Board {
  public:
	enum EBoardType { kBoardTypeUnknown = 0, kBoardTypeBRD = 0x01, kBoardTypeBDV = 0x02 };

	virtual ~Board() {}

	virtual SharedVector<Net> &Nets()                               = 0;
	virtual SharedVector<Component> &Components()                   = 0;
	virtual SharedVector<Pin> &Pins()                               = 0;
	virtual SharedVector<Point> &OutlinePoints()                    = 0;
	virtual std::vector<std::pair<Point, Point>> &OutlineSegments() = 0;

	EBoardType BoardType() {
		return kBoardTypeUnknown;
	}
};
