#pragma once

#include <stdlib.h>

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
};

struct BRDNail {
    int probe;
    BRDPoint pos;
    int side;
    char *net;
};

struct BRDFile {
	int num_format;
	int num_parts;
	int num_pins;
	int num_nails;

	BRDPoint *format;
	BRDPart *parts;
	BRDPin *pins;
	BRDNail *nails;

	char *file_buf;

	bool valid;

	BRDFile(const char *buf, size_t buffer_size);
	~BRDFile() {
		free(file_buf);
		free(format);
	}
};
