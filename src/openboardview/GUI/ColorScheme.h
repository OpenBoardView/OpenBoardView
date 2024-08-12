#ifndef _COLORSCHEME_H_
#define _COLORSCHEME_H_

#include <cstdint>

#include "confparse.h"

class ColorScheme {
public:
	/*
	 * Take note, because these are directly set
	 * the packing format is ABGR,  not RGBA
	 */
	uint32_t backgroundColor          = 0xffffffff;
	uint32_t boardFillColor           = 0xffdddddd;
	uint32_t partOutlineColor         = 0xff444444;
	uint32_t partHullColor            = 0x80808080;
	uint32_t partFillColor            = 0xffffffff;
	uint32_t partTextColor            = 0x80808080;
	uint32_t partHighlightedColor     = 0xff0000ee;
	uint32_t partHighlightedFillColor = 0xf4f0f0ff;
	uint32_t partHighlightedTextColor            = 0xff808000;
	uint32_t partHighlightedTextBackgroundColor  = 0xff00eeee;
	uint32_t boardOutlineColor        = 0xff00ffff;

	//	uint32_t boxColor = 0xffcccccc;

	uint32_t pinDefaultColor      = 0xff0000ff;
	uint32_t pinDefaultTextColor  = 0xffcc0000;
	uint32_t pinTextBackgroundColor  = 0xffffff80;
	uint32_t pinGroundColor       = 0xff0000bb;
	uint32_t pinNotConnectedColor = 0xffff0000;
	uint32_t pinTestPadColor      = 0xff888888;
	uint32_t pinTestPadFillColor  = 0xff8dc6d6;
	uint32_t pinA1PadColor        = 0xffdd0000;

	uint32_t pinSelectedColor     = 0x00000000;
	uint32_t pinSelectedFillColor = 0xffff8888;
	uint32_t pinSelectedTextColor = 0xffffffff;

	uint32_t pinSameNetColor     = 0xffaa4040;
	uint32_t pinSameNetFillColor = 0xffff9999;
	uint32_t pinSameNetTextColor = 0xff111111;

	uint32_t pinHaloColor     = 0x8822FF22;
	uint32_t pinNetWebColor   = 0xff0000ff;
	uint32_t pinNetWebOSColor = 0x0000ff22;

	uint32_t annotationPartAliasColor       = 0xcc00ffff;
	uint32_t annotationBoxColor             = 0xaa0000ff;
	uint32_t annotationStalkColor           = 0xff000000;
	uint32_t annotationPopupBackgroundColor = 0xffeeeeee;
	uint32_t annotationPopupTextColor       = 0xff000000;

	uint32_t selectedMaskPins    = 0xFFFFFFFF;
	uint32_t selectedMaskParts   = 0xFFFFFFFF;
	uint32_t selectedMaskOutline = 0xFFFFFFFF;

	uint32_t orMaskPins    = 0x00000000;
	uint32_t orMaskParts   = 0x00000000;
	uint32_t orMaskOutline = 0x00000000;

	static uint32_t byte4swap(uint32_t x);

	void ThemeSetStyle(const char *name);
	void readFromConfig(Confparse &obvconfig);
	void writeToConfig(Confparse &obvconfig);
};

#endif//_COLORSCHEME_H_
