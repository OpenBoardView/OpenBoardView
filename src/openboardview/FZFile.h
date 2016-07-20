#pragma once

#include "BRDFile.h"

#undef LOAD_INT
#undef LOAD_DOUBLE
#undef LOAD_STR
#define LOAD_INT(var)        \
	var = strtol(p, &p, 10); \
	if (*p == '!') p++;
#define LOAD_DOUBLE(var) \
	var = strtod(p, &p); \
	if (*p == '!') p++;
#define LOAD_STR(var)                            \
	while (isspace((uint8_t)*p)) ++p;            \
	s = p;                                       \
	while (*p != '!') /* '!' is our delimiter */ \
		++p;                                     \
	*p++ = 0;                                    \
	var  = fix_to_utf8(s, &arena, arena_end);

class FZFile : public BRDFile {
  public:
	FZFile(const char *buf, size_t buffer_size, uint32_t *fzkey);
	~FZFile() {
		free(file_buf);
	}

	void SetKey(char *keytext);

  private:
	static void decode(char *source, size_t size);
	static char *split(char *file_buf, size_t buffer_size, size_t &content_size);
	static char *decompress(char *file_buf, size_t buffer_size, size_t &output_size);
	void gen_outline();
	void update_counts();

	// Put your key here.
	// uint32_t keylength = 2*r + 4; // i.e. buf[0..2r+3]
	// static constexpr uint32_t key[44] = {0};
	// static uint32_t key[44] = {0};
	static uint32_t key[44];
};
