#include "BVRFile.h"

#include "utf8/utf8.h"
#include <assert.h>
#include <ctype.h>
#include <locale.h>
#include <stdint.h>
#include <string.h>

char *nextfield(char *p) {
	if (p) {
		while ((*p != '\t') && (*p != '\0') && (*p != '\r') && (*p != '\n')) {
			p++;
		}
	}
	if (*p == '\t') p++;
	return p;
}

BVRFile::BVRFile(const char *buf, size_t buffer_size) {
	char *saved_locale;
	char ppn[100] = {0};                        // previous part name
	saved_locale  = setlocale(LC_NUMERIC, "C"); // Use '.' as delimiter for strtod

	memset(this, 0, sizeof(*this));
#define ENSURE(X) \
	assert(X);    \
	if (!(X)) return;

#define FAIL_LABEL fail
	ENSURE(buffer_size > 4);
	size_t file_buf_size = 3 * (1 + buffer_size);
	file_buf             = (char *)malloc(file_buf_size);
	memcpy(file_buf, buf, buffer_size);
	file_buf[buffer_size] = 0;
	// This is for fixing degenerate utf8
	char *arena     = &file_buf[buffer_size + 1];
	char *arena_end = file_buf + file_buf_size - 1;
	*arena_end      = 0;

	int current_block = 0;

	char **lines = stringfile(file_buf);
	if (!lines) return;
	//		goto fail;
	char **lines_begin = lines;
#undef FAIL_LABEL
#define FAIL_LABEL fail_lines
	while (*lines) {
		char *line = *lines;
		++lines;

		while (isspace((uint8_t)*line)) line++;
		if (!line[0]) continue;

		if (!strcmp(line, "<<Layout>>")) {
			//			fprintf(stderr,"HIT LAYOUT\n");
			current_block = 1;
			lines += 1; // Skip 1 unused lines before 1st layout
			continue;
		}
		if (!strcmp(line, "<<Pin>>")) {
			current_block = 2;
			//			fprintf(stderr,"HIT PIN, block = %d\n", current_block);
			lines += 1; // Skip 1 unused lines before 1st pin
			continue;
		}
		if (!strcmp(line, "<<Nail>>")) {
			//			fprintf(stderr,"HIT NAIL\n");
			current_block = 3;
			lines += 1; // Skip 1 unused lines before 1st nail
			continue;
		}

		char *p = line;
		char *s;

		switch (current_block) {
			case 1: { // Format
				BRDPoint point;
				double x;
				//				fprintf(stderr,"Decoding format ");
				LOAD_DOUBLE(x);
				point.x = trunc(x * 1000); // OBV uses integers
				if (*p == ',') p++;
				double y;
				LOAD_DOUBLE(y);
				point.y = trunc(y * 1000);
				format.push_back(point);
			} break;

			case 2: { // Parts & Pins
				BRDPart part;
				BRDPin pin;

				LOAD_STR(part.name);

				char *loc;
				LOAD_STR(loc);
				if (!strcmp(loc, "(T)"))
					part.type = 10; // SMD part on top
				else
					part.type = 5; // SMD part on bottom

				// If this is the first time we've seen this part
				if ((strcmp(ppn, part.name))) {
					part.end_of_pins = 0;
					parts.push_back(part);
					snprintf(ppn, sizeof(ppn), "%s", part.name);
				}

				pin.part = parts.size(); // the part this pin is associated with, is the last part on the vector

				int id;
				LOAD_INT(id);

				char *name;
				LOAD_STR(name);

				double posx;
				LOAD_DOUBLE(posx);
				pin.pos.x = trunc(posx * 1000);

				double posy;
				LOAD_DOUBLE(posy);
				pin.pos.y = trunc(posy * 1000);

				int layer;
				LOAD_INT(layer);

				LOAD_STR(pin.net);

				// LOAD_INT(pin.probe);
				//

				pins.push_back(pin);

				parts.back().end_of_pins = pins.size();
			} break;

			case 3: { // Nails
				BRDNail nail;

				p = nextfield(p);
				double posx;
				LOAD_DOUBLE(posx);
				nail.pos.x = trunc(posx * 1000);
				//				fprintf(stderr,"%d ",nail.pos.x);

				double posy;
				LOAD_DOUBLE(posy);
				nail.pos.y = trunc(posy * 1000);
				//				fprintf(stderr,"%d ",nail.pos.y);

				int type;
				LOAD_INT(type);
				//				fprintf(stderr,"Type:%d ",type);

				char *grid;
				LOAD_STR(grid);
				//				fprintf(stderr,"Grid:%s ", grid);

				char *loc;
				LOAD_STR(loc);
				if (!strcmp(loc, "(T)"))
					nail.side = 1;
				else
					nail.side = 2;
				//				fprintf(stderr,"Side:%d ",nail.side);

				char *netid;
				LOAD_STR(netid);
				//				fprintf(stderr,"ID:%s ", netid);

				LOAD_STR(nail.net);
				//				fprintf(stderr,"Net:%s\n", nail.net);

				nail.pos.x = posx * 1000;
				nail.pos.y = posy * 1000;

				nails.push_back(nail);
				// LOAD_INT(nail.probe);
				//
			} break;

			default: continue;
		}
	}

	num_parts  = parts.size();
	num_pins   = pins.size();
	num_format = format.size();
	num_nails  = nails.size();

	setlocale(LC_NUMERIC, saved_locale); // Restore locale

	valid = current_block != 0;
fail_lines:
	free(lines_begin);
fail:;
#undef FAIL_LABEL
#undef ENSURE
}
