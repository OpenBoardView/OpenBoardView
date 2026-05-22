#ifndef __CONFPARSE__
#define __CONFPARSE__
#include <string>

#include "filesystem_impl.h"

#define CONFPARSE_MAX_VALUE_SIZE 10240
struct Confparse {

	filesystem::path filepath;
	char value[CONFPARSE_MAX_VALUE_SIZE];
	char *conf, *limit;
	size_t buffer_size;
	bool nested = false;

	~Confparse(void);
	int Load(const filesystem::path &filepath, bool save_default = false);
	int SaveDefault(const filesystem::path &filepath);
	char *Parse(const char *key);
	const char *ParseStr(const char *key, const char *defaultv);
	double ParseDouble(const char *key, double defaultv);
	int ParseInt(const char *key, int defaultv);
	bool ParseBool(const char *key, bool defaultv);
	uint32_t ParseHex(const char *key, uint32_t defaultv);

	bool WriteStr(const char *key, const char *value);
	bool WriteBool(const char *key, bool value);
	bool WriteInt(const char *key, int value);
	bool WriteHex(const char *key, uint32_t value);
	bool WriteFloat(const char *key, double value);
};

#endif
