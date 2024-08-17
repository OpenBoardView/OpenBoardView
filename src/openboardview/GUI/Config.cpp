#include "Config.h"

#include <cstring>
#include "confparse.h"
#include "GUI/DPI.h"

void Config::SetFZKey(const char *keytext) {

	if (keytext) {
		FZKeyStr = keytext;

		int ki;
		const char *p, *limit;
		char *ep;
		ki    = 0;
		p     = keytext;
		limit = keytext + strlen(keytext);

		if ((limit - p) > 440) {
			/*
			 * we *assume* that the key is correctly formatted in the configuration file
			 * as such it should be like FZKey = 0x12345678, 0xabcd1234, ...
			 *
			 * If your key is incorrectly formatted, or incorrect, it'll cause OBV to
			 * likely crash / segfault (for now).
			 */
			while (p && (p < limit) && ki < 44) {

				// locate the start of the u32 hex value
				while ((p < limit) && (*p != '0')) p++;

				// decode the next number, ep will be set to the end of the converted string
				FZKey[ki] = strtoll(p, &ep, 16);

				ki++;
				p = ep;
			}
		}
	}
}

void Config::readFromConfig(Confparse &obvconfig) {
	// Special test here, in case we've already set the dpi from external
	// such as command line.
	if (getDPI() == 0) dpi = obvconfig.ParseInt("dpi", 100);
	if (dpi < 50) dpi = 50;
	if (dpi > 400) dpi = 400;

	windowX = obvconfig.ParseInt("windowX", 1100);
	windowY = obvconfig.ParseInt("windowY", 700);

	fontSize            = obvconfig.ParseDouble("fontSize", 20);
	fontName            = obvconfig.ParseStr("fontName", "");
	pinSizeThresholdLow = obvconfig.ParseDouble("pinSizeThresholdLow", 0);
	pinShapeSquare      = obvconfig.ParseBool("pinShapeSquare", false);
	pinShapeCircle      = obvconfig.ParseBool("pinShapeCircle", true);

	if ((!pinShapeCircle) && (!pinShapeSquare)) {
		pinShapeSquare = true;
	}


	pinHalo          = obvconfig.ParseBool("pinHalo", true);
	pinHaloDiameter  = obvconfig.ParseDouble("pinHaloDiameter", 1.25);
	pinHaloThickness = obvconfig.ParseDouble("pinHaloThickness", 2.0);
	pinSelectMasks   = obvconfig.ParseBool("pinSelectMasks", true);

	pinA1threshold	  = obvconfig.ParseInt("pinA1threshold", 3);

	showFPS                   = obvconfig.ParseBool("showFPS", false);
	showInfoPanel             = obvconfig.ParseBool("showInfoPanel", true);
	infoPanelSelectPartsOnNet = obvconfig.ParseBool("infoPanelSelectPartsOnNet", true);
	infoPanelCenterZoomNets   = obvconfig.ParseBool("infoPanelCenterZoomNets", true);
	partZoomScaleOutFactor    = obvconfig.ParseDouble("partZoomScaleOutFactor", 3.0f);

	infoPanelWidth            = obvconfig.ParseInt("infoPanelWidth", 350);
	showPins                  = obvconfig.ParseBool("showPins", true);
	showPosition              = obvconfig.ParseBool("showPosition", true);
	showNetWeb                = obvconfig.ParseBool("showNetWeb", true);
	showAnnotations           = obvconfig.ParseBool("showAnnotations", true);
	showBackgroundImage       = obvconfig.ParseBool("showBackgroundImage", true);
	fillParts                 = obvconfig.ParseBool("fillParts", true);
	showPartName              = obvconfig.ParseBool("showPartName", true);
	showPinName               = obvconfig.ParseBool("showPinName", true);
	centerZoomSearchResults = obvconfig.ParseBool("centerZoomSearchResults", true);
	flipMode                  = obvconfig.ParseInt("flipMode", 0);

	boardFill        = obvconfig.ParseBool("boardFill", true);
	boardFillSpacing = obvconfig.ParseInt("boardFillSpacing", 3);

	zoomFactor   = obvconfig.ParseDouble("zoomFactor", 0.5f);
	zoomModifier = obvconfig.ParseInt("zoomModifier", 5);

	panFactor = obvconfig.ParseInt("panFactor", 30);

	panModifier = obvconfig.ParseInt("panModifier", 5);

	annotationBoxSize = obvconfig.ParseInt("annotationBoxSize", 15);

	annotationBoxOffset = obvconfig.ParseInt("annotationBoxOffset", 8);

	netWebThickness = obvconfig.ParseInt("netWebThickness", 2);


#ifdef _WIN32
	pdfSoftwarePath = obvconfig.ParseStr("pdfSoftwarePath", "SumatraPDF.exe");
#endif

	/*
	 * Some machines (Atom etc) don't have enough CPU/GPU
	 * grunt to cope with the large number of AA'd circles
	 * generated on a large dense board like a Macbook Pro
	 * so we have the lowCPU option which will let people
	 * trade good-looks for better FPS
	 *
	 * If we want slowCPU to take impact from a command line
	 * parameter, we need it to be set to false before we
	 * call this.
	 */
	slowCPU |= obvconfig.ParseBool("slowCPU", false);

	/*
	 * The asus .fz file formats require a specific key to be decoded.
	 *
	 * This key is supplied in the obv.conf file as a long single line
	 * of comma/space separated 32-bit hex values 0x1234abcd etc.
	 *
	 */
	SetFZKey(obvconfig.ParseStr("FZKey", ""));
}

void Config::writeToConfig(Confparse &obvconfig) {
	obvconfig.WriteInt("dpi", dpi);
	obvconfig.WriteInt("windowX", windowX);
	obvconfig.WriteInt("windowY", windowY);

	obvconfig.WriteFloat("fontSize", fontSize);
	obvconfig.WriteStr("fontName", fontName.c_str());
	obvconfig.WriteFloat("pinSizeThresholdLow", pinSizeThresholdLow);
	obvconfig.WriteBool("pinShapeSquare", pinShapeSquare);
	obvconfig.WriteBool("pinShapeCircle", pinShapeCircle);

	obvconfig.WriteBool("pinHalo", pinHalo);
	obvconfig.WriteFloat("pinHaloDiameter", pinHaloDiameter);
	obvconfig.WriteFloat("pinHaloThickness", pinHaloThickness);
	obvconfig.WriteBool("pinSelectMasks", pinSelectMasks);

	obvconfig.WriteInt("pinA1threshold", pinA1threshold);

	obvconfig.WriteBool("showFPS", showFPS);
	obvconfig.WriteBool("showInfoPanel", showInfoPanel);
	obvconfig.WriteBool("infoPanelSelectPartsOnNet", infoPanelSelectPartsOnNet);
	obvconfig.WriteBool("infoPanelCenterZoomNets", infoPanelCenterZoomNets);
	obvconfig.WriteFloat("partZoomScaleOutFactor", partZoomScaleOutFactor);

	obvconfig.WriteInt("infoPanelWidth", infoPanelWidth);
	obvconfig.WriteBool("showPins", showPins);
	obvconfig.WriteBool("showPosition", showPosition);
	obvconfig.WriteBool("showNetWeb", showNetWeb);
	obvconfig.WriteBool("showAnnotations", showAnnotations);
	obvconfig.WriteBool("showBackgroundImage", showBackgroundImage);
	obvconfig.WriteBool("fillParts", fillParts);
	obvconfig.WriteBool("showPartName", showPartName);
	obvconfig.WriteBool("showPinName", showPinName);
	obvconfig.WriteBool("centerZoomSearchResults", centerZoomSearchResults);
	obvconfig.WriteInt("flipMode", flipMode);

	obvconfig.WriteBool("boardFill", boardFill);
	obvconfig.WriteInt("boardFillSpacing", boardFillSpacing);

	obvconfig.WriteFloat("zoomFactor", zoomFactor);
	obvconfig.WriteInt("zoomModifier", zoomModifier);

	obvconfig.WriteInt("panFactor", panFactor);

	obvconfig.WriteInt("panModifier", panModifier);

	obvconfig.WriteInt("annotationBoxSize", annotationBoxSize);

	obvconfig.WriteInt("annotationBoxOffset", annotationBoxOffset);

	obvconfig.WriteInt("netWebThickness", netWebThickness);


#ifdef _WIN32
	obvconfig.WriteStr("pdfSoftwarePath", pdfSoftwarePath);
#endif

	obvconfig.WriteBool("slowCPU", slowCPU);

	obvconfig.WriteStr("FZKey", FZKeyStr.c_str());
}
