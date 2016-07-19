
#define CONFPARSE_MAX_VALUE_SIZE 10240
struct Confparse {

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
};
