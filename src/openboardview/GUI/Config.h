#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <array>
#include <cstdint>
#include <string>

#include "confparse.h"

class Config {
public:
	int windowX = 1200;
	int windowY = 700;
	int dpi = 100;
	double fontSize              = 20.0;
	std::string fontName         = "";
	float zoomFactor             = 0.5f;
	float partZoomScaleOutFactor = 3.0f;
	int zoomModifier             = 5;
	int panFactor                = 30;
	int panModifier              = 5;
	int flipMode                 = 0;

	int annotationBoxOffset = 8;
	int annotationBoxSize   = 20;

	int pinA1threshold = 3; // pincount of package to show 1/A1 pin
	int netWebThickness = 2;

	float pinSizeThresholdLow = 0.0f;
	bool pinShapeSquare       = false;
	bool pinShapeCircle       = true;
	bool pinSelectMasks       = true;
	bool slowCPU              = false;
	bool showFPS              = false;
	bool showNetWeb           = true;
	bool showInfoPanel        = true;
	int infoPanelWidth        = 300;
	bool showPins             = true;
	bool showAnnotations      = true;
	bool showBackgroundImage  = true;
	bool pinHalo              = false;
	float pinHaloDiameter     = 1.1;
	float pinHaloThickness    = 4.00;
	bool fillParts            = true;
	bool boardFill            = true;
	bool showPartName         = true;
	bool showPinName          = true;
	int boardFillSpacing      = 3;
	bool showPosition  = true;

	bool infoPanelCenterZoomNets   = true;
	bool infoPanelSelectPartsOnNet = true;
	bool centerZoomSearchResults = true;

	template<size_t N>
	std::array<uint32_t, N> DecodeKey(const char *keytext);

	std::string FZKeyStr = "";
	std::array<uint32_t, 44> FZKey = {0};
	void SetFZKey(const char *keytext);

	std::string CAEKeyStr = "";
	std::array<uint32_t, 44> CAEKey = {0};
	void SetCAEKey(const char *keytext);

	void readFromConfig(Confparse &obvconfig);
	void writeToConfig(Confparse &obvconfig);
};

#endif//_CONFIG_H_
