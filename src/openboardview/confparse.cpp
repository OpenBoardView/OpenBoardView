#include <errno.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "confparse.h"
#include "version.h"

/*
 * XRayBlue theme - by Inflex
 */
char default_conf[] =
    "#\r\n\
# " OBV_NAME
    " configuration\r\n\
#\r\n\
# Renderer options\r\n\
#  1 = OpenGL1\r\n\
#  2 = OpenGL3\r\n\
#  3 = OpenGLES2\r\n\
renderer=2\r\n\
\r\n\
windowX=1200\r\n\
windowY=700\r\n\
\r\n\
# Reference DPI is 100, increase if you have a higher density (ie, small 4K or 2K screen)\r\n\
dpi=100\r\n\
\r\n\
fontName = \r\n\
fontSize = 20\r\n\
showInfoPanel = true\r\n\
infoPanelWidth = 300\r\n\
showPins = true\r\n\
showNetWeb = true\r\n\
pinSelectMasks = true\r\n\
pinSizeThresholdLow = 0\r\n\
pinShapeCircle = true\r\n\
pinShapeSquare = false\r\n\
\r\n\
slowCPU =       false\r\n\
showFPS =       false\r\n\
pinHalo =       false\r\n\
pinHaloDiameter = 1.1\r\n\
pinHaloThickness = 4\r\n\
\r\n\
fillParts =		true\r\n\
boardFill =		true\r\n\
boardFillSpacing = 3\r\n\
\r\n\
zoomFactor = 5\r\n\
zoomModifier = 5\r\n\
\r\n\
panFactor = 30\r\n\
panModifier = 5\r\n\
\r\n\
centerZoomSearchResults = true\r\n\
infoPanelCenterZoomNets = true\r\n\
infoPanelSelectPartsOnNet = true\r\n\
partZoomScaleOutFactor = 3.0\r\n\
\r\n\
# Flip board modes\r\n\
#  0: flip whole board in view port, shift-flip to flip around mouse ptr\r\n\
#  1: flip around mouse ptr, shift-flip to flip view port\r\n\
flipMode = 0\r\n\
\r\n\
showAnnotations = true\r\n\
annotationBoxSize = 20\r\n\
annotationBoxOffset = 8\r\n\
#\r\n\
# \"XRayBlue\" Theme by Inflex (20160724)\r\n\
# Colors, format is 0xRRGGBBAA\r\n\
#\r\n\
# There's two built in themes, light (default) and dark \r\n\
#colorTheme = default\r\n\
#colorTheme = dark\r\n\
colorTheme = light\r\n\
backgroundColor		= 0xffffffff\r\n\
boardFillColor	= 0xddddddff\r\n\
partOutlineColor = 0x444444ff\r\n\
partHullColor			= 0x80808080\r\n\
partFillColor = 0xffffffbb\r\n\
partHighlightedFillColor = 0xf4f0f0ff\r\n\
partHighlightedColor = 0xff0000ee\r\n\
partTextColor			= 0xff3030ff\r\n\
partTextBackgroundColor			= 0xffff00ff\r\n\
\r\n\
# Pin colourings.\r\n\
#  default is for pins that aren't selected\r\n\
#  selected is for the actual clicked on pin\r\n\
#  highlighted is for pins usually on the same network as the selected\r\n\
#\r\n\
# There's an absense of 'fill' colours on most because the CPU hit is\r\n\
# moderately high to do them all \r\n\
#\r\n\
boardOutlineColor			= 0x444444ff\r\n\
pinDefaultColor				= 0x8888ccff\r\n\
pinDefaultTextColor			= 0x666688ff\r\n\
pinGroundColor				= 0x2222aaff\r\n\
pinNotConnectedColor		= 0xaaaaaaff\r\n\
pinTestPadColor				= 0x888888ff\r\n\
pinTestPadFillColor				= 0xbd9e2dff\r\n\
\r\n\
pinSelectedColor				= 0x00000000\r\n\
pinSelectedFillColor			= 0x8888ffff\r\n\
pinSelectedTextColor			= 0xffffffff\r\n\
\r\n\
pinSameNetColor			= 0x0000ffff\r\n\
pinSameNetFillColor		= 0x9999ffff\r\n\
pinSameNetTextColor		= 0x111111ff\r\n\
\r\n\
pinHaloColor			= 0x22FF2288\r\n\
\r\n\
pinNetWebColor = 0xff000044\r\n\
pinNetWebOSColor = 0x0000ff33\r\n\
\r\n\
annotationPopupTextColor = 0x000000ff\r\n\
annotationPopupBackgroundColor = 0xeeeeeeff\r\n\
annotationBoxColor = 0xff0000aa\r\n\
annotationStalkColor = 0x000000ff\r\n\
\r\n\
selectedMaskPins		= 0xffffffff\r\n\
selectedMaskParts		= 0xffffffff\r\n\
selectedMaskOutline		= 0xffffffff\r\n\
\r\n\
orMaskPins		= 0x00000000\r\n\
orMaskParts		= 0x00000000\r\n\
orMaskOutline	= 0x00000000\r\n\
# EndColors\r\n\
\r\n\
# FZKey requires 44 32-bit values in order for it to work.\r\n\
#  If you have the key, put it in here as a single line, each value comma separated\r\n\
#FZKey = 0x12345678, 0x12345678\r\n\
FZKey =   \r\n\
\r\n\
# END OF CONF\r\n";

/*
 * Original dark theme
 *
 */
/*
char default_conf[] =
    "#\r\n\
# " OBV_NAME " configuration file (inflex-ui)\r\n\
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

Confparse::~Confparse(void) {
	if (conf) free(conf);
}

int Confparse::SaveDefault(const std::string &utf8_filename) {
	std::ofstream file;
	file.open(utf8_filename, std::ios::out | std::ios::binary | std::ios::ate);

	nested = true;
	if (file.is_open()) {
		file.write(default_conf, sizeof(default_conf) - 1);
		file.close();
		Load(utf8_filename);

		return 0;
	} else {
		return 1;
	}
}

int Confparse::Load(const std::string &utf8_filename) {
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

	filename = utf8_filename;

	if (conf) free(conf);

	std::streampos sz = file.tellg();
	buffer_size       = sz;
	conf              = (char *)calloc(1, buffer_size + 1);
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

	if (!conf[0]) return NULL;
	if (!key[0]) return NULL;

	keylen = strlen(key);
	if (keylen == 0) return NULL;

	p = strstr(conf, key);
	if (p == NULL) {
		return NULL;
	}

	op = p;

	llimit = limit; // - keylen - 2; // allows for up to 'key=x'

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

		// if the previous strstr() was a non-success, then try search again from the next bit
		if (op < limit) {
			p = strstr(op + 1, key);
			if (!p) break;
			op = p;
		} else {
			break;
		}
	}
	return NULL;
}

const char *Confparse::ParseStr(const char *key, const char *defaultv) {
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

uint32_t Confparse::ParseHex(const char *key, uint32_t defaultv) {
	char *p = Parse(key);
	if (p) {
		uint32_t v;
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
		double v = strtod(p, NULL);
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

/*
 * Write parts
 *
 */
bool Confparse::WriteStr(const char *key, const char *value) {
	char *p, *op;
	char *llimit;
	int keylen;

	if (!conf) return false;
	if (filename.empty()) return false;
	if (!value) return false;
	if (!key) return false;

	keylen = strlen(key);
	if (keylen == 0) return false;

	op = p = strstr(conf, key);
	if (p == NULL) {
		char buf[1024];
		size_t bs;
		const std::string nfn = filename + "~";
		std::ofstream file;

		rename(filename.c_str(), nfn.c_str());
		file.open(filename, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!file.is_open()) {
			return false;
		}
		file.write(conf, buffer_size); // write the leadup
		bs = snprintf(buf, sizeof(buf), "\r\n%s = %s", key, value);
		file.write(buf, bs);
		file.flush();
		file.close();

		Load(filename);
		return true;
	}

	//	llimit = limit - keylen - 2; // allows for up to 'key=x'
	llimit = limit;
	while (p && p < llimit) {

		/*
		 * Consume the name<whitespace>=<whitespace> trash before we
		 * get to the actual value.  While this does mean things are
		 * slightly less strict in the configuration file text it can
		 * assist in making it easier for people to read it.
		 */
		p = p + keylen;
		if ((p < llimit) && (!isalnum(*p))) {

			while ((p < llimit) && ((*p == '=') || (*p == ' ') || (*p == '\t'))) {
				p++; // get up to the start of the value;
			}

			if ((p < llimit) && (p >= conf)) {

				/*
				 * Check that the location of the strstr() return is
				 * left aligned to the start of the line. This prevents
				 * us picking up trash name=value pairs among the general
				 * text within the file
				 */
				if ((op == conf) || (*(op - 1) == '\r') || (*(op - 1) == '\n')) {
					char *ep = NULL;

					/*
					 * op represents the start of the line/key.
					 * p represents the start of the value.
					 *
					 * we can just dump out to the file up to p and then
					 * print our data
					 */

					/*
					 * Search for the end of the data by finding the end of the line
					 * This will become 'ep', which we'll use as the start of the
					 * rest of the file data we dump back to the config.
					 */
					ep = p;
					while ((ep < limit) && ((*ep != '\0') && (*ep != '\n') && (*ep != '\r'))) {
						ep++;
					}

					{
						const std::string nfn = filename + "~";
						std::ofstream file;

						rename(filename.c_str(), nfn.c_str());
						file.open(filename, std::ios::out | std::ios::binary | std::ios::trunc);
						if (!file.is_open()) {
							return false;
						}
						file.write(conf, (p - conf));     // write the leadup
						file.write(value, strlen(value)); // write the new data
						file.write(ep, limit - ep);       // write the rest of the file
						file.close();

						Load(filename);
						return true;
					}
				}
			}
		}
		p  = strstr(op + 1, key);
		op = p;
	}

	/*
	 * If we didn't find our parameter in the config file, then add it.
	 */
	{
		char sep[CONFPARSE_MAX_VALUE_SIZE];
		std::ofstream file;
		file.open(filename, std::ios::out | std::ios::binary | std::ios::app);
		if (!file.is_open()) {
			return false;
		}
		snprintf(sep, sizeof(sep), "%s = %s\r\n", key, value);
		file.write(sep, strlen(sep));
		file.flush();
		file.close();

		Load(filename);
		return true;
	}

	return false;
}

bool Confparse::WriteBool(const char *key, bool value) {
	char v[10];
	snprintf(v, sizeof(v), "%s", value ? "true" : "false");
	return WriteStr(key, v);
};

bool Confparse::WriteInt(const char *key, int value) {
	char v[20];
	snprintf(v, sizeof(v), "%d", value);
	return WriteStr(key, v);
};
bool Confparse::WriteHex(const char *key, uint32_t value) {
	char v[20];
	snprintf(v, sizeof(v), "0x%08x", value);
	return WriteStr(key, v);
};
bool Confparse::WriteFloat(const char *key, double value) {
	char v[20];
	snprintf(v, sizeof(v), "%f", value);
	return WriteStr(key, v);
};
