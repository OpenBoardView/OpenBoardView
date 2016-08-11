#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "history.h"

FHistory::~FHistory() {

	if (fname) free(fname);
}

int FHistory::Set_filename(const char *f) {
#ifdef _WIN32
	fname = _strdup(f);
#else
	fname = strdup(f);
#endif
	return 0;
}

int FHistory::Load(void) {
	if (fname) {
		FILE *f;
#ifdef _WIN32
		errno_t e;
		e = fopen_s(&f, fname, "r");
#else
		f = fopen(fname, "r");
#endif

		count = 0;
		if (!f) return 0;

		while (count < FHISTORY_COUNT_MAX) {
			char *r;

			r = fgets(history[count], FHISTORY_FNAME_LEN_MAX, f);
			if (r) {
				count++;

				/// strip off the trailing line break
				while (*r) {
					if ((*r == '\r') || (*r == '\n')) {
						*r = '\0';
						break;
					}
					r++;
				}

			} else {
				break;
			}
		}
		fclose(f);
	} else {
		return -1;
	}

	return count;
}

int FHistory::Prepend_save(char *newfile) {
	if (fname) {
		FILE *f;
#ifdef _WIN32
		errno_t e;

		e = fopen_s(&f, fname, "w");
#else
		f = fopen(fname, "w");
#endif

		if (f) {
			int i;

			fprintf(f, "%s\n", newfile);
			for (i = 0; i < count; i++) {
				// Don't create duplicate entries, so check each one against the newfile
				if (strcmp(newfile, history[i])) {
					fprintf(f, "%s\n", history[i]);
				}
			}
			fclose(f);

			Load();
		}
	}

	return 0;
}

/**
 * Only displays the tail end of the filename path, where
 * 'stops' indicates how many paths up to truncate at
 *
 * PLD20160618-1729
 */
char *FHistory::Trim_filename(char *s, int stops) {

	int l   = strlen(s);
	char *p = s + l - 1;

	while ((stops) && (p > s)) {
		if ((*p == '/') || (*p == '\\')) stops--;
		p--;
	}
	if (!stops) p += 2;

	return p;
}
