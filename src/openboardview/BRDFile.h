#pragma once


#include "Board.h"
#include <stdlib.h>


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
