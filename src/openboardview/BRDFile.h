#pragma once

#include <array>
#include <stdlib.h>
#include <string>
#include <vector>

#define LOAD_INT(var) var    = strtol(p, &p, 10)
#define LOAD_DOUBLE(var) var = strtod(p, &p);
#define LOAD_STR(var)                            \
	while ((*p) && (isspace((uint8_t)*p))) ++p;  \
	s = p;                                       \
	while ((*p) && (!isspace((uint8_t)*p))) ++p; \
	*p = 0;                                      \
	p++;                                         \
	var = fix_to_utf8(s, &arena, arena_end);

struct BRDPoint {
	int x;
	int y;
};

struct BRDPart {
	char *name;
	int type;
	int end_of_pins;
};

struct BRDPin {
	BRDPoint pos;
	int probe;
	int part;
	char *net;
	double radius = 0.5f;
	char *snum    = nullptr;

	bool operator<(const BRDPin &p) const // For sorting the vector
	{
		return part == p.part ? (std::string(snum) < std::string(p.snum)) : (part < p.part); // sort by part number then pin number
	}
};

struct BRDNail {
	int probe;
	BRDPoint pos;
	int side;
	char *net;
};

class BRDFile {
  public:
	int num_format;
	int num_parts;
	int num_pins;
	int num_nails;

	std::vector<BRDPoint> format;
	std::vector<BRDPart> parts;
	std::vector<BRDPin> pins;
	std::vector<BRDNail> nails;

	char *file_buf;

	bool valid;

	BRDFile(const char *buf, size_t buffer_size);
	BRDFile(){};
	~BRDFile() {
		free(file_buf);
	}

	static bool verifyFormat(const char *buf, size_t buffer_size);

  private:
	static constexpr std::array<uint8_t, 4> signature = {0x23, 0xe2, 0x63, 0x28};
};

char **stringfile(char *buffer);
char *fix_to_utf8(char *s, char **arena, char *arena_end);
