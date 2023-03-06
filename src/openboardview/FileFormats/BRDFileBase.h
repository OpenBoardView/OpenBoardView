#pragma once

#include <cstdlib>
#include <string>
#include <vector>

#define READ_INT() strtol(p, &p, 10);
// Warning: read as int then cast to uint if positive
#define READ_UINT                                \
	[&]() {                                      \
		int value = strtol(p, &p, 10);           \
		ENSURE(value >= 0, error_msg);           \
		return static_cast<unsigned int>(value); \
	}
#define READ_DOUBLE() strtod(p, &p)
#define READ_STR                                     \
	[&]() {                                          \
		while ((*p) && (isspace((uint8_t)*p))) ++p;  \
		s = p;                                       \
		while ((*p) && (!isspace((uint8_t)*p))) ++p; \
		*p = 0;                                      \
		p++;                                         \
		return fix_to_utf8(s, &arena, arena_end);    \
	}

struct BRDPoint {
	// mil (thou) is used here
	int x = 0;
	int y = 0;

	// Declaring these constructors wouldn't be required in C++17
	BRDPoint() = default;
	BRDPoint(int x, int y) : x(x), y(y) {}

	bool operator==(const BRDPoint& point) {
		return x == point.x && y == point.y;
	}
	bool operator!=(const BRDPoint& point) {
		return x != point.x || y != point.y;
	}
};

enum class BRDPartMountingSide { Both, Bottom, Top };
enum class BRDPartType { SMD, ThroughHole };

struct BRDPart {
	const char *name = nullptr;
	std::string mfgcode;
	BRDPartMountingSide mounting_side{};
	BRDPartType part_type{}; // SMD or TH
	unsigned int end_of_pins = 0;
	BRDPoint p1{0, 0};
	BRDPoint p2{0, 0};
};

enum class BRDPinSide { Both, Bottom, Top };

struct BRDPin {
	BRDPoint pos;
	int probe = 0;
	unsigned int part = 0;
	BRDPinSide side{};
	const char *net = "UNCONNECTED";
	double radius    = 0.5f;
	const char *snum = nullptr;
	const char *name = nullptr;

	struct LessByPartAndNumberAndName {
		bool operator()(const BRDPin &a, const BRDPin &b) const;
	};
};

struct BRDNail {
	unsigned int probe = 0;
	BRDPoint pos;
	BRDPartMountingSide side{};
	const char *net = "UNCONNECTED";
};

class BRDFileBase {
  public:
	unsigned int num_format = 0;
	unsigned int num_parts  = 0;
	unsigned int num_pins   = 0;
	unsigned int num_nails  = 0;

	std::vector<BRDPoint> format;
	std::vector<std::pair<BRDPoint, BRDPoint>> outline_segments;
	std::vector<BRDPart> parts;
	std::vector<BRDPin> pins;
	std::vector<BRDNail> nails;

	bool valid = false;
	std::string error_msg = "";

	virtual ~BRDFileBase()
	{
		free(file_buf);
		file_buf = nullptr;
	}
  protected:
	void AddNailsAsPins();
	BRDFileBase() {}
	// file_buf is used by some implementations. But since the derived class constructurs
	// are already passed a memory buffer most usages are "historic unneeded extra copies".
	char *file_buf = nullptr;
};

void stringfile(char *buffer, std::vector<char*> &lines);
char *fix_to_utf8(char *s, char **arena, char *arena_end);
