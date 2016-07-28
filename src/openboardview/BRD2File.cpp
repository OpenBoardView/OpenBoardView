#include "BRD2File.h"

#include "utf8/utf8.h"
#include <assert.h>
#include <ctype.h>
#include <locale.h>
#include <stdint.h>
#include <string.h>

#define LOAD_INT(var) var    = strtol(p, &p, 10)
#define LOAD_DOUBLE(var) var = strtod(p, &p);
#define LOAD_STR(var)                            \
	while ((*p) && (isspace((uint8_t)*p))) ++p;  \
	s = p;                                       \
	while ((*p) && (!isspace((uint8_t)*p))) ++p; \
	*p = 0;                                      \
	p++;                                         \
	var = fix_to_utf8(s, &arena, arena_end);

BRD2File::BRD2File(const char *buf, size_t buffer_size) {
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

		//		fprintf(stderr,"Parsing:%s\n",line);

		while (isspace((uint8_t)*line)) line++;
		if (!line[0]) continue;

		if (strstr(line, "BRDOUT:") == line) {
			current_block = 1;
			continue;
		}
		if (strstr(line, "NETS:") == line) {
			current_block = 2;
			continue;
		}
		if (strstr(line, "PARTS:") == line) {
			current_block = 3;
			continue;
		}
		if (strstr(line, "PINS:") == line) {
			current_block = 4;
			continue;
		}
		if (strstr(line, "NAILS:") == line) {
			current_block = 4;
			continue;
		}

		char *p = line;
		char *s;

		switch (current_block) {
			case 1: { // Format
				BRDPoint point;

				LOAD_INT(point.x);
				LOAD_INT(point.y);
				format.push_back(point);
			} break;

			case 2: { // Nets
				      /*
			  BRDNet net;
			  LOAD_INT(net.number);
			  LOAD_STR(net.name);
			  if (strcmp(net.name, "GND") == 0)
				  net.is_ground = true;
			  else
				  net.is_ground = false;
			  Nets.push_back(net);
			  */
			} break;

			case 3: { // PARTS
				BRDPart part;
				int dud, side;

				LOAD_STR(part.name);
				LOAD_INT(dud);
				LOAD_INT(dud);
				LOAD_INT(dud);
				LOAD_INT(dud);
				LOAD_INT(dud);
				LOAD_INT(part.end_of_pins);
				LOAD_INT(side);
				parts.push_back(part);
			} break;

			case 4: { // PINS
				BRDPin pin;
				int pindex, side;

				LOAD_INT(pin.pos.x);
				LOAD_INT(pin.pos.y);
				LOAD_INT(pindex);
				LOAD_INT(side);
				pins.push_back(pin);
			} break;

			case 5: { // NAILS
				BRDNail nail;
				int nindex;
				LOAD_INT(nail.probe);
				LOAD_INT(nail.pos.x);
				LOAD_INT(nail.pos.y);
				LOAD_INT(nindex);
				LOAD_INT(nail.side);
				nails.push_back(nail);

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
