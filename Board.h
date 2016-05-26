#pragma once

#include "BRDFile.h"

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

using namespace std;

struct Point;
struct BoardElement;
struct Net;
struct Pin;
struct Component;

typedef function<void(const char *)> TcharStringCallback;
typedef function<void(BoardElement *)> TboardElementCallback;

template <class T>
using SharedVector = vector<shared_ptr<T>>;

template <class T>
using SharedStringMap = map<string, shared_ptr<T>>;

enum EBoardSide {
	kBoardSideTop = 0,
	kBoardSideBottom = 1,
	kBoardSideBoth = 2,
};

// Checking whether str `prefix` is a prefix of str `base`.
inline static bool is_prefix(string prefix, string base) {
	if (prefix.size() > base.size())
		return false;

	auto res = mismatch(prefix.begin(), prefix.end(), base.begin());
	if (res.first == prefix.end())
		return true;

	return false;
}

template <class T>
inline static bool contains(const T &element, vector<T *> &v) {
	return find(begin(v), end(v), &element) != end(v);
}

template <class T>
inline static bool contains(T &element, vector<T *> &v) {
	return find(begin(v), end(v), &element) != end(v);
}

// Any element being on the board.
struct BoardElement {
	// Side of the board the element is located. (top, bottom, both?)
	EBoardSide board_side = kBoardSideBoth;

	// String uniquely identifying this element on the board.
	virtual string UniqueId() const = 0;
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
	string name;
	bool is_ground;

	vector<Pin *> pins;

	string UniqueId() const {
		return kBoardNetPrefix + name;
	}
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
	int number;

	// Position according to board file. (probably in inches)
	Point position;

	// Contact diameter, e.g. via or pin size. (probably in inches)
	float diameter;

	// Net this contact is connected to, nulltpr if no info available.
	Net *net;

	// Contact belonging to this component (pin), nullptr if nail.
	Component *component;

	string UniqueId() const {
		return kBoardPinPrefix + std::to_string(number);
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
		kComponentTypeJellyBean
	};

	// How the part is attached to the board, either SMD, .., through-hole?
	EMountType mount_type = kMountTypeUnknown;

	// Type of component, eg. resistor, cap, etc.
	EComponentType component_type = kComponentTypeUnknown;

	// Part name as stored in board file.
	string name;

	// Pins belonging to this component.
	vector<Pin *> pins;

	// Mount type as readable string.
	string mount_type_str() {
		switch (mount_type) {
		case Component::kMountTypeSMD:
			return "SMD";
		case Component::kMountTypeDIP:
			return "DIP";
		default:
			return "UNKNOWN";
		}
	}

	// true if component is not representing a real/physical component.
	bool is_dummy() {
		return component_type == kComponentTypeDummy;
	}

	string UniqueId() const {
		return kBoardComponentPrefix + name;
	}
};

class Board {
  public:
	enum EBoardType { kBoardTypeUnknown = 0, kBoardTypeBRD = 0x01, kBoardTypeBDV = 0x02 };

	virtual ~Board() {
	}

	virtual SharedVector<Net> &Nets() = 0;
	virtual SharedVector<Component> &Components() = 0;
	virtual SharedVector<Pin> &Pins() = 0;
	virtual SharedVector<Point> &OutlinePoints() = 0;

	EBoardType BoardType() {
		return kBoardTypeUnknown;
	}
};
