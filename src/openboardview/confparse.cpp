#include <errno.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "confparse.h"

/*
 * XRayBlue theme - by Inflex
 */
char default_conf[] =
    "#\r\n\
# OpenFlex Board View configuration file (inflex-ui)\r\n\
#\r\n\
windowX=900\r\n\
windowY=600\r\n\
#fontPath=FiraSans-Medium.ttf\r\n\
fontSize=20\r\n\
pinSizeThresholdLow = 0\r\n\
pinShapeCircle = true\r\n\
pinShapeSquare = false\r\n\
\r\n\
slowCPU =       false\r\n\
showFPS =       false\r\n\
pinHalo =       true\r\n\
fillParts =		true\r\n\
\r\n\
zoomFactor = 5\r\n\
zoomModifier = 5\r\n\
\r\n\
panFactor = 30\r\n\
panModifier = 5\r\n\
\r\n\
annotationBoxSize = 10\r\n\
annotationBoxOffset = 15\r\n\
annotationBoxColor = 0xff0000aa\r\n\
annotationStalkColor = 0x000000ff\r\n\
#\r\n\
# \"XRayBlue\" Theme by Inflex (20160724)\r\n\
# Colors, format is 0xRRGGBBAA\r\n\
#\r\n\
lightTheme = true\r\n\
backgroundColor		= 0xeeeeeeff\r\n\
partTextColor			= 0xff3030ff\r\n\
partTextBackgroundColor			= 0xffff00ff\r\n\
boardOutline			= 0x444444ff\r\n\
boxColor					= 0x444444ff\r\n\
pinDefault				= 0x8888ffff\r\n\
pinGround				= 0x2222aaff\r\n\
pinNotConnected		= 0xaaaaaaff\r\n\
pinTestPad				= 0x888888ff\r\n\
pinSelected				= 0x0000ffff\r\n\
pinSelectedText			= 0xff0000ff\r\n\
pinHighlighted			= 0x0000ffff\r\n\
pinHaloColor			= 0x00aa00ff\r\n\
pinHighlightSameNet	= 0x000000ff\r\n\
annotationPartAlias	= 0xffff00ff\r\n\
partHullColor			= 0x80808080\r\n\
\r\n\
selectedMaskPins		= 0xffffffff\r\n\
selectedMaskParts		= 0xffffffff\r\n\
selectedMaskOutline	= 0xffffffff\r\n\
\r\n\
orMaskPins		= 0xccccccff\r\n\
orMaskParts		= 0x787878ff\r\n\
orMaskOutline	= 0x888888ff\r\n\
# EndColors\r\n\
\r\n\
FZKey = 0x12345678, 0x12345678\r\n\
";

/*
 * Original dark theme
 *
 */
/*
char default_conf[] =
    "#\r\n\
# OpenFlex Board View configuration file (inflex-ui)\r\n\
#\r\n\
windowX=1280\r\n\
windowY=900\r\n\
#fontPath=FiraSans-Medium.ttf\r\n\
fontSize=20\r\n\
pinSizeThresholdLow = 0\r\n\
pinShapeCircle = true\r\n\
pinShapeSquare = false\r\n\
\r\n\
slowCPU =       false\r\n\
showFPS =       false\r\n\
pinHalo =       true\r\n\
fillParts =		true\r\n\
\r\n\
zoomFactor = 5\r\n\
zoomModifier = 5\r\n\
\r\n\
panFactor = 30\r\n\
panModifier = 5\r\n\
\r\n\
# Colors, format is 0xRRGGBBAA\r\n\
lightTheme	= false\r\n\
backgroundColor		= 0x000000a0\r\n\
partTextColor			= 0x008080ff\r\n\
partTextBackgroundColor	= 0xeeee00ff\r\n\
boardOutline			= 0xffff00ff\r\n\
boxColor					= 0xccccccff\r\n\
pinDefault				= 0xff0000ff\r\n\
pinGround				= 0xbb0000ff\r\n\
pinNotConnected		= 0x0000ffff\r\n\
pinTestPad				= 0x888888ff\r\n\
pinSelected				= 0xeeeeeeff\r\n\
pinHighlighted			= 0xffffffff\r\n\
pinHaloColor			= 0x00ff006f\r\n\
pinHighlightSameNet	= 0xfff888ff\r\n\
annotationPartAlias	= 0xffff00ff\r\n\
partHullColor			= 0x80808080\r\n\
selectedMaskPins		= 0xffffff4f\r\n\
selectedMaskParts		= 0xffffff8f\r\n\
selectedMaskOutline	= 0xffffff8f\r\n\
\r\n\
# OR mask pins are applied after the above selectedMask, in the \r\n\
# form of color = original & selectedMask | orMask\r\n\
#\r\n\
# This is useful if you want to *lighten* a colour when 'masked'\r\n\
#\r\n\
orMaskPins		= 0x00000000\r\n\
orMaskParts		= 0x00000000\r\n\
orMaskOutline	= 0x00000000\r\n\
# EndColors\r\n\
\r\n\
#FZKey = 0x12345678, 0x12345678, 0x12345678, 0x12345678, 0x12345678, 0x12345678, 0x12345678, 0x12345678, 0x12345678, 0x12345678,
0x12345678, 0x12345678, 0x12345678, 0x12345678, 0x12345678, 0x12345678, 0x12345678, 0x12345678, 0x12345678, 0x12345678,
0x12345678\r\n\
\r\n\
# END\r\n\
";
*/

int Confparse::SaveDefault(const char *utf8_filename) {
	std::ofstream file;
	file.open(utf8_filename, std::ios::out | std::ios::binary | std::ios::ate);

	nested = true;
	if (file.is_open()) {
		file.write(default_conf, sizeof(default_conf));
		file.close();
		Load(utf8_filename);

		return 0;
	} else
		return 1;
}

int Confparse::Load(const char *utf8_filename) {
	std::ifstream file;
	file.open(utf8_filename, std::ios::in | std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		//		std::cerr << "Error opening " << utf8_filename << ": " <<
		// strerror(errno) << std::endl;
		buffer_size = 0;
		conf        = NULL;
		if (nested) return 1; // to prevent infinite recursion, we test the nested flag
		return (SaveDefault(utf8_filename));
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
	//
	//

	nested = false;

	return 0;
}

char *Confparse::Parse(const char *key) {
	char *p, *op;
	char *llimit;
	int keylen;

	value[0] = '\0';

	if (!conf) return NULL;
	if (!key) return NULL;

	keylen = strlen(key);
	if (keylen == 0) return NULL;

	op = p = strstr(conf, key);
	if (p == NULL) return NULL;

	llimit = limit - keylen - 2; // allows for up to 'key=x'

	while (p && p < llimit) {

		/*
		 * Consume the name<whitespace>=<whitespace> trash before we
		 * get to the actual value.  While this does mean things are
		 * slightly less strict in the configuration file text it can
		 * assist in making it easier for people to read it.
		 */
		p = p + keylen;

		if ((p < llimit) && (!isalnum(*p))) {

			while ((p < llimit) && ((*p == '=') || (*p == ' ') || (*p == '\t'))) p++; // get up to the start of the value;

			if ((p < llimit) && (p >= conf)) {

				/*
				 * Check that the location of the strstr() return is
				 * left aligned to the start of the line. This prevents
				 * us picking up trash name=value pairs among the general
				 * text within the file
				 */
				if ((op == conf) || (*(op - 1) == '\r') || (*(op - 1) == '\n')) {
					size_t i = 0;

					/*
					 * Search for the end of the data by finding the end of the line
					 */
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
		p  = strstr(op + 1, key);
		op = p;
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

unsigned long Confparse::ParseHex(const char *key, unsigned long defaultv) {
	char *p = Parse(key);
	if (p) {
		unsigned long v;
		if ((*p == '0') && (*(p + 1) == 'x')) {
			p += 2;
		}
		v = strtoul(p, NULL, 16);
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

bool Confparse::ParseBool(const char *key, bool defaultv) {
	char *p = Parse(key);
	if (p) {
		if (strncmp(p, "true", sizeof("true")) == 0) {
			return true;
		} else
			return false;
	} else
		return defaultv;
}
