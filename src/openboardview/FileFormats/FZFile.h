#pragma once

#include "BRDFile.h"

#undef READ_INT
#undef READ_UINT
#undef READ_DOUBLE
#undef READ_STR
/* '!' is our delimiter */
#define READ_INT                       \
	[&]() {                            \
		int value = strtol(p, &p, 10); \
		if (*p == '!') p++;            \
		return value;                  \
	}
// Warning: read as int then cast to uint if positive
#define READ_UINT                                \
	[&]() {                                      \
		int value = strtol(p, &p, 10);           \
		if (*p == '!') p++;                      \
		ENSURE(value >= 0);                      \
		return static_cast<unsigned int>(value); \
	}
#define READ_DOUBLE                 \
	[&]() {                         \
		double val = strtod(p, &p); \
		if (*p == '!') p++;         \
		return val;                 \
	}
#define READ_STR                                    \
	[&]() {                                         \
		while ((*p) && (isspace((uint8_t)*p))) ++p; \
		s = p;                                      \
		while ((*p) && (*p != '!')) ++p;            \
		*p = 0;                                     \
		p++;                                        \
		return fix_to_utf8(s, &arena, arena_end);   \
	}

class FZFile : public BRDFile {
  public:
	FZFile(std::vector<char> &buf, uint32_t *fzkey);
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
