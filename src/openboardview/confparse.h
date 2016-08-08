#ifndef __CONFPARSE__
#define __CONFPARSE__
#define CONFPARSE_MAX_VALUE_SIZE 10240
#define CONFPARSE_FILENAME_MAX 1024
struct Confparse {

	char filename[CONFPARSE_FILENAME_MAX];
	char value[CONFPARSE_MAX_VALUE_SIZE];
	char *conf, *limit;
	size_t buffer_size;
	bool nested = false;

	int Load(const char *utf8_filename);
	int SaveDefault(const char *utf8_filename);
	char *Parse(const char *key);
	char *ParseStr(const char *key, char *defaultv);
	double ParseDouble(const char *key, double defaultv);
	int ParseInt(const char *key, int defaultv);
	bool ParseBool(const char *key, bool defaultv);
	unsigned long ParseHex(const char *key, unsigned long defaultv);

	bool WriteStr(const char *key, char *value);
	bool WriteBool(const char *key, bool value);
	bool WriteInt(const char *key, int value);
	bool WriteHex(const char *key, unsigned long value);
	bool WriteFloat(const char *key, double value);
};

#endif
