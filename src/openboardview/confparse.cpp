#include <errno.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "confparse.h"

int Confparse::Load(const char *utf8_filename) {
	std::ifstream file;
	file.open(utf8_filename, std::ios::in | std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		std::cerr << "Error opening " << utf8_filename << ": " << strerror(errno) << std::endl;
		buffer_size = 0;
		return 1;
	}

	std::streampos sz = file.tellg();
	buffer_size       = sz;
	conf              = (char *)malloc(sz);
	file.seekg(0, std::ios::beg);
	file.read(conf, sz);
	limit = conf + sz;
	file.close();

	if (file.gcount() != sz) {
		std::cerr << "Did not read the right number of bytes from configuration file" << std::endl;
		return 1;
	}
	//	assert(file.gcount() == sz);

	return 0;
}

char *Confparse::Parse(const char *key) {
	char *p;
	char *llimit;
	int keylen;

	value[0] = '\0';

	if (!conf) return NULL;
	if (!key) return NULL;

	keylen = strlen(key);
	p      = strstr(conf, key);

	llimit = limit - keylen - 2; // allows for up to 'key=x'

	while (p && p < llimit) {
		if (*(p + keylen) == '=') {
			if ((p < llimit) && (p >= conf)) {
				if ((p == conf) || (*(p - 1) == '\r') || (*(p - 1) == '\n')) {
					size_t i = 0;

					p = p + keylen + 1;
					while ((p < limit) && ((*p != '\0') && (*p != '\n') && (*p != '\r'))) {
						value[i] = *p;
						p++;
						i++;
						if (i >= CONFPARSE_MAX_VALUE_SIZE) break;
					}
					value[i] = '\0';

					return value;
				}
			}
		}
		p = strstr(p + 1, key);
	}
	return NULL;
}

char *Confparse::ParseStr(const char *key, char *defaultv) {
	char *p = Parse(key);
	if (p)
		return p;
	else
		return defaultv;
}

int Confparse::ParseInt(const char *key, int defaultv) {
	char *p = Parse(key);
	if (p) {
		int v = strtol((char *)Parse(key), NULL, 10);
		if (errno == ERANGE)
			return defaultv;
		else
			return v;
	} else
		return defaultv;
}

double Confparse::ParseDouble(const char *key, double defaultv) {
	char *p = Parse(key);
	if (p) {
		double v = strtof((char *)Parse(key), NULL);
		if (errno == ERANGE)
			return defaultv;
		else
			return v;

	} else
		return defaultv;
}
