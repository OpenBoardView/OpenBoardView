#include "platform.h" // Should be kept first
#include "BoardView.h"
#include "history.h"
#include "utf8/utf8.h"
#include "utils.h"
#include "version.h"
#include "imgui_operators.h"

#include <cmath>
#include <iostream>
#include <climits>
#include <memory>
#include <cstdio>
#ifdef ENABLE_SDL2
#include <SDL.h>
#endif

#include "BRDBoard.h"
#include "Board.h"
#include "FileFormats/ADFile.h"
#include "FileFormats/ASCFile.h"
#include "FileFormats/BDVFile.h"
#include "FileFormats/BRD2File.h"
#include "FileFormats/BRDFile.h"
#include "FileFormats/BVRFile.h"
#include "FileFormats/CADFile.h"
//#include "FileFormats/CAMCADFile.h"
#include "FileFormats/CSTFile.h"
#include "FileFormats/FZFile.h"
#include "annotations.h"
#include "imgui/imgui.h"

#include "TCL.h"
#include "NetList.h"
#include "PartList.h"
#include "vectorhulls.h"

using namespace std;
using namespace std::placeholders;

#if _MSC_VER
#define stricmp _stricmp
#endif

#ifndef _WIN32
#define stricmp strcasecmp
#endif

BoardView::~BoardView() {
	if (m_validBoard) {
		for (auto &p : m_board->Components()) {
			if (p->hull) free(p->hull);
		}
		m_board->OutlinePoints().clear();

		if constexpr (! std::is_same<obv_shared_ptr<Component>, std::shared_ptr<Component> >::value) {
			delete m_file.get();
			delete m_board.get();
			m_file = nullptr;
			m_board = nullptr;
		} else {
			m_board->Nets().clear();
			m_board->Pins().clear();
			m_board->Components().clear();
		}
		m_annotations.Close();
		m_validBoard = false;
	}
}
uint32_t BoardView::byte4swap(uint32_t x) {
	/*
	 * used to convert RGBA -> ABGR etc
	 */
	return (((x & 0x000000ff) << 24) | ((x & 0x0000ff00) << 8) | ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24));
}

template <typename F>
inline void cfg_color_visit_each(ColorScheme & c, F f) {
	// config key name            ColorScheme field name      dark        light
	f("backgroundColor",          c.backgroundColor,          0x000000ff, 0xffffffff);
	f("boardFillColor",           c.boardFillColor,           0x2a2a2aff, 0xddddddff);
	f("partHullColor",            c.partHullColor,            0x80808080, 0x80808080);
	f("partOutlineColor",         c.partOutlineColor,         0x999999ff, 0x444444ff);
	f("partFillColor",            c.partFillColor,            0x111111ff, 0xffffff77);
	f("partHighlightedColor",     c.partHighlightedColor,     0xffffffff, 0xff0000ff);
	f("partHighlightedFillColor", c.partHighlightedFillColor, 0x333333ff, 0xf0f0f0ff);
	f("partTextColor",            c.partTextColor,            0x000000ff, 0xff3030ff);
	f("partTextBackgroundColor",  c.partTextBackgroundColor,  0xcccc22ff, 0xffff00ff);
	f("boardOutlineColor",        c.boardOutlineColor,        0xcc2222ff, 0x444444ff);
	f("pinDefaultColor",          c.pinDefaultColor,          0x4040ffff, 0x22aa33ff);
	f("pinDefaultTextColor",      c.pinDefaultTextColor,      0xccccccff, 0x666688ff);
	f("pinGroundColor",           c.pinGroundColor,           0x0300c0ff, 0x2222aaff);
	f("pinNotConnectedColor",     c.pinNotConnectedColor,     0xaaaaaaff, 0xaaaaaaff);
	f("pinTestPadColor",          c.pinTestPadColor,          0x888888ff, 0x888888ff);
	f("pinTestPadFillColor",      c.pinTestPadFillColor,      0x6c5b1fff, 0xd6c68dff);
	f("pinA1PadColor",            c.pinA1PadColor,            0xdd0000ff, 0xdd0000ff);
	f("pinSelectedTextColor",     c.pinSelectedTextColor,     0xffffffff, 0xffffffff);
	f("pinSelectedFillColor",     c.pinSelectedFillColor,     0x8888ffff, 0x8888ffff);
	f("pinSelectedColor",         c.pinSelectedColor,         0x00ff00ff, 0x00000000);
	f("pinSameNetTextColor",      c.pinSameNetTextColor,      0x111111ff, 0x111111ff);
	f("pinSameNetFillColor",      c.pinSameNetFillColor,      0x9999ffff, 0x9999ffff);
	f("pinSameNetColor",          c.pinSameNetColor,          0x0000ffff, 0x888888ff);
	f("pinHaloColor",             c.pinHaloColor,             0xffffff88, 0x22ff2288);
	f("pinNetWebColor",           c.pinNetWebColor,           0x8888ff88, 0x0000ff33);
	f("annotationPartAliasColor", c.annotationPartAliasColor, 0xffff00ff, 0xffff00ff);
	f("annotationBoxColor",       c.annotationBoxColor,       0xcccc88ff, 0xff0000aa);
	f("annotationStalkColor",     c.annotationStalkColor,     0xaaaaaaff, 0x000000ff);
	f("annotationPopupBackgroundColor", c.annotationPopupBackgroundColor, 0x888888ff, 0xeeeeeeff);
	f("annotationPopupTextColor", c.annotationPopupTextColor, 0xffffffff, 0x000000ff);
	f("selectedMaskPins",         c.selectedMaskPins,         0xffffffff, 0xffffffff);
	f("selectedMaskParts",        c.selectedMaskParts,        0xffffffff, 0xffffffff);
	f("selectedMaskOutline",      c.selectedMaskOutline,      0xffffffff, 0xffffffff);
	f("orMaskPins",               c.orMaskPins,               0x00000000, 0x00000000);
	f("orMaskParts",              c.orMaskParts,              0x00000000, 0x00000000);
	f("orMaskOutline",            c.orMaskOutline,            0x00000000, 0x00000000);
}	

void BoardView::ThemeSetStyle(const char *name) {
	ImGuiStyle &style = ImGui::GetStyle();

	// non color-related settings
	style.WindowBorderSize = 0.0f;

	bool dark = strcmp(name, "dark") == 0;
	auto f = [&style, dark](ImGuiCol icol,
					 float dr, float dg, float db, float da,
					 float lr, float lg, float lb, float la) {
		if (dark) {
			style.Colors[icol] = ImVec4(dr, dg, db, da);
		} else {
			style.Colors[icol] = ImVec4(lr, lg, lb, la);
		}
	};

	f(ImGuiCol_Text,                 0.90f, 0.90f, 0.90f, 1.00f,   0.00f, 0.00f, 0.00f, 1.00f);
	f(ImGuiCol_TextDisabled,         0.60f, 0.60f, 0.60f, 1.00f,   0.60f, 0.60f, 0.60f, 1.00f);
	f(ImGuiCol_WindowBg,             0.00f, 0.00f, 0.00f, 0.70f,   0.94f, 0.94f, 0.94f, 1.00f);
	f(ImGuiCol_ChildBg,              0.00f, 0.00f, 0.00f, 0.00f,   0.00f, 0.00f, 0.00f, 0.00f);
	f(ImGuiCol_PopupBg,              0.05f, 0.05f, 0.10f, 0.90f,   0.94f, 0.94f, 0.94f, 1.00f);
	f(ImGuiCol_Border,               0.70f, 0.70f, 0.70f, 0.65f,   0.00f, 0.00f, 0.00f, 0.39f);
	f(ImGuiCol_BorderShadow,         0.00f, 0.00f, 0.00f, 0.00f,   1.00f, 1.00f, 1.00f, 0.10f);
	f(ImGuiCol_FrameBg, 		     0.30f, 0.30f, 0.30f, 1.00f,   1.00f, 1.00f, 1.00f, 1.00f); // Background of checkbox, radio button, plot, slider, text input
	f(ImGuiCol_FrameBgHovered,       0.90f, 0.80f, 0.80f, 0.40f,   0.26f, 0.59f, 0.98f, 0.40f);
	f(ImGuiCol_FrameBgActive,        0.90f, 0.65f, 0.65f, 0.45f,   0.26f, 0.59f, 0.98f, 0.67f);
	f(ImGuiCol_TitleBg,              0.27f, 0.27f, 0.54f, 0.83f,   0.96f, 0.96f, 0.96f, 1.00f);
	f(ImGuiCol_TitleBgCollapsed,     0.40f, 0.40f, 0.80f, 0.20f,   1.00f, 1.00f, 1.00f, 0.51f);
	f(ImGuiCol_TitleBgActive,        0.32f, 0.32f, 0.63f, 0.87f,   0.82f, 0.82f, 0.82f, 1.00f);
	f(ImGuiCol_MenuBarBg,            0.40f, 0.40f, 0.55f, 0.80f,   0.82f, 0.82f, 0.82f, 1.00f);
	f(ImGuiCol_ScrollbarBg,          0.20f, 0.25f, 0.30f, 0.60f,   0.98f, 0.98f, 0.98f, 0.53f);
	f(ImGuiCol_ScrollbarGrab,        0.40f, 0.40f, 0.80f, 0.30f,   0.69f, 0.69f, 0.69f, 0.80f);
	f(ImGuiCol_ScrollbarGrabHovered, 0.40f, 0.40f, 0.80f, 0.40f,   0.49f, 0.49f, 0.49f, 0.80f);
	f(ImGuiCol_ScrollbarGrabActive,  0.80f, 0.50f, 0.50f, 0.40f,   0.49f, 0.49f, 0.49f, 1.00f);
	f(ImGuiCol_CheckMark,            0.90f, 0.90f, 0.90f, 0.50f,   0.26f, 0.59f, 0.98f, 1.00f);
	f(ImGuiCol_SliderGrab,           1.00f, 1.00f, 1.00f, 0.30f,   0.26f, 0.59f, 0.98f, 0.78f);
	f(ImGuiCol_SliderGrabActive,     0.80f, 0.50f, 0.50f, 1.00f,   0.26f, 0.59f, 0.98f, 1.00f);
	f(ImGuiCol_Button,               0.67f, 0.40f, 0.40f, 0.60f,   0.26f, 0.59f, 0.98f, 0.40f);
	f(ImGuiCol_ButtonHovered,        0.67f, 0.40f, 0.40f, 1.00f,   0.26f, 0.59f, 0.98f, 1.00f);
	f(ImGuiCol_ButtonActive,         0.80f, 0.50f, 0.50f, 1.00f,   0.06f, 0.53f, 0.98f, 1.00f);
	f(ImGuiCol_Header,               0.40f, 0.40f, 0.90f, 0.45f,   0.26f, 0.59f, 0.98f, 0.31f);
	f(ImGuiCol_HeaderHovered,        0.45f, 0.45f, 0.90f, 0.80f,   0.26f, 0.59f, 0.98f, 0.80f);
	f(ImGuiCol_HeaderActive,         0.53f, 0.53f, 0.87f, 0.80f,   0.26f, 0.59f, 0.98f, 1.00f);
	f(ImGuiCol_Separator,            0.50f, 0.50f, 0.50f, 1.00f,   0.39f, 0.39f, 0.39f, 1.00f);
	f(ImGuiCol_SeparatorHovered,     0.70f, 0.60f, 0.60f, 1.00f,   0.26f, 0.59f, 0.98f, 0.78f);
	f(ImGuiCol_SeparatorActive,      0.90f, 0.70f, 0.70f, 1.00f,   0.26f, 0.59f, 0.98f, 1.00f);
	f(ImGuiCol_ResizeGrip,           1.00f, 1.00f, 1.00f, 0.30f,   1.00f, 1.00f, 1.00f, 0.00f);
	f(ImGuiCol_ResizeGripHovered,    1.00f, 1.00f, 1.00f, 0.60f,   0.26f, 0.59f, 0.98f, 0.67f);
	f(ImGuiCol_ResizeGripActive,     1.00f, 1.00f, 1.00f, 0.90f,   0.26f, 0.59f, 0.98f, 0.95f);
	f(ImGuiCol_PlotLines,            1.00f, 1.00f, 1.00f, 1.00f,   0.39f, 0.39f, 0.39f, 1.00f);
	f(ImGuiCol_PlotLinesHovered,     0.90f, 0.70f, 0.00f, 1.00f,   1.00f, 0.43f, 0.35f, 1.00f);
	f(ImGuiCol_PlotHistogram,        0.90f, 0.70f, 0.00f, 1.00f,   0.90f, 0.70f, 0.00f, 1.00f);
	f(ImGuiCol_PlotHistogramHovered, 1.00f, 0.60f, 0.00f, 1.00f,   1.00f, 0.60f, 0.00f, 1.00f);
	f(ImGuiCol_TextSelectedBg,       0.00f, 0.00f, 1.00f, 0.35f,   0.26f, 0.59f, 0.98f, 0.35f);
	f(ImGuiCol_ModalWindowDarkening, 0.20f, 0.20f, 0.20f, 0.35f,   0.20f, 0.20f, 0.20f, 0.35f);
	f(ImGuiCol_TableRowBg,           0.05f, 0.05f, 0.10f, 0.90f,   0.94f, 0.94f, 0.94f, 1.00f);
	f(ImGuiCol_TableRowBgAlt,        0.10f, 0.10f, 0.20f, 0.90f,   0.82f, 0.82f, 0.82f, 1.00f);
	
	if (dark) {
		cfg_color_visit_each(m_colors, [](const char *, uint32_t & c, uint32_t dark, uint32_t) {
								 c = byte4swap(dark);
							 });
	} else {
		cfg_color_visit_each(m_colors, [](const char *, uint32_t & c, uint32_t, uint32_t lite) {
								 c = byte4swap(lite);
							 });
	}
}

void BoardView::set_tcl(TCL * t) {
	m_tcl = t;
	if (t) {
		auto p = m_tcl->keybind_targets();
		if (!p.empty()) {
			tcl_keybindings = new KeyBindings(&p);
			tcl_keyboardPreferences = new Preferences::Keyboard(*tcl_keybindings, obvconfig);
			tcl_keybindings->readFromConfig(obvconfig);
		}
	}		
}


int BoardView::ConfigParse(void) {
	ImGuiStyle &style = ImGui::GetStyle();
	const char *v     = obvconfig.ParseStr("colorTheme", "light");
	ThemeSetStyle(v);

	fontSize            = obvconfig.ParseDouble("fontSize", 20);
	pinSizeThresholdLow = obvconfig.ParseDouble("pinSizeThresholdLow", 0);
	pinShapeSquare      = obvconfig.ParseBool("pinShapeSquare", false);
	pinShapeCircle      = obvconfig.ParseBool("pinShapeCircle", true);

	if ((!pinShapeCircle) && (!pinShapeSquare)) {
		pinShapeSquare = true;
	}

	// Special test here, in case we've already set the dpi from external
	// such as command line.
	if (!dpi) dpi = obvconfig.ParseInt("dpi", 100);
	if (dpi < 50) dpi = 50;
	if (dpi > 400) dpi = 400;
	dpiscale = dpi / 100.0f;

	pinHalo          = obvconfig.ParseBool("pinHalo", true);
	pinHaloDiameter  = obvconfig.ParseDouble("pinHaloDiameter", 1.25);
	pinHaloThickness = obvconfig.ParseDouble("pinHaloThickness", 2.0);
	pinSelectMasks   = obvconfig.ParseBool("pinSelectMasks", true);

	startFullscreen  = obvconfig.ParseBool("startFullscreen", false);
	
	pinA1threshold	  = obvconfig.ParseInt("pinA1threshold", 3);

	showFPS                   = obvconfig.ParseBool("showFPS", false);
	showInfoPanel             = obvconfig.ParseBool("showInfoPanel", true);
	infoPanelSelectPartsOnNet = obvconfig.ParseBool("infoPanelSelectPartsOnNet", true);
	infoPanelCenterZoomNets   = obvconfig.ParseBool("infoPanelCenterZoomNets", true);
	partZoomScaleOutFactor    = obvconfig.ParseDouble("partZoomScaleOutFactor", 3.0f);

	m_info_surface.x          = obvconfig.ParseInt("infoPanelWidth", 350);
	showPins                  = obvconfig.ParseBool("showPins", true);
	showPosition              = obvconfig.ParseBool("showPosition", true);
	showNetWeb                = obvconfig.ParseBool("showNetWeb", true);
	showAnnotations           = obvconfig.ParseBool("showAnnotations", true);
	backgroundImage.enabled   = obvconfig.ParseBool("showBackgroundImage", true);
	fillParts                 = obvconfig.ParseBool("fillParts", true);
	m_centerZoomSearchResults = obvconfig.ParseBool("centerZoomSearchResults", true);
	flipMode                  = obvconfig.ParseInt("flipMode", 0);

	boardFill        = obvconfig.ParseBool("boardFill", true);
	boardFillSpacing = obvconfig.ParseInt("boardFillSpacing", 3);

	zoomFactor   = obvconfig.ParseInt("zoomFactor", 10) / 10.0f;
	zoomModifier = obvconfig.ParseInt("zoomModifier", 5);

	panFactor = obvconfig.ParseInt("panFactor", 30);
	panFactor = DPI(panFactor);

	panModifier = obvconfig.ParseInt("panModifier", 5);

	annotationBoxSize = obvconfig.ParseInt("annotationBoxSize", 15);
	annotationBoxSize = DPI(annotationBoxSize);

	annotationBoxOffset = obvconfig.ParseInt("annotationBoxOffset", 8);
	annotationBoxOffset = DPI(annotationBoxOffset);

	netWebThickness = obvconfig.ParseInt("netWebThickness", 2);

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
	style.AntiAliasedLines = !slowCPU;
	style.AntiAliasedFill  = !slowCPU;

	/*
	 * Colours in ImGui can be represented as a 4-byte packed uint32_t as ABGR
	 * but most humans are more accustomed to RBGA, so for the sake of readability
	 * we use the human-readable version but swap the ordering around when
	 * it comes to assigning the actual colour to ImGui.
	 */

	cfg_color_visit_each(m_colors, [this](const char * cfg, uint32_t & c, uint32_t, uint32_t) {
							 c = byte4swap(obvconfig.ParseHex(cfg, byte4swap(c)));
						 });
	/*
	 * The asus .fz file formats require a specific key to be decoded.
	 *
	 * This key is supplied in the obv.conf file as a long single line
	 * of comma/space separated 32-bit hex values 0x1234abcd etc.
	 *
	 */
	SetFZKey(obvconfig.ParseStr("FZKey", ""));

	keybindings.readFromConfig(obvconfig);
	return 0;
}

BRDFile * BoardView::loadBoard(const filesystem::path &filepath) {
	std::vector<char> buffer = file_as_buffer(filepath);
	if (!buffer.empty()) {
		BRDFile *file = nullptr;
		
		if (check_fileext(filepath, ".fz")) { // Since it is encrypted we cannot use the below logic. Trust the ext.
			file = new FZFile(buffer, FZKey);
		} else if (check_fileext(filepath, ".bom") || check_fileext(filepath, ".asc"))
			file = new ASCFile(buffer, filepath);
		else if (ADFile::verifyFormat(buffer))
			file = new ADFile(buffer);
		else if (CADFile::verifyFormat(buffer))
			file = new CADFile(buffer);
		//			else if (CAMCADFile::verifyFormat(buffer))
		//	file = new CAMCADFile(buffer);
		else if (check_fileext(filepath, ".cst"))
			file = new CSTFile(buffer);
		else if (BRDFile::verifyFormat(buffer))
			file = new BRDFile(buffer);
		else if (BRD2File::verifyFormat(buffer))
			file = new BRD2File(buffer);
		else if (BDVFile::verifyFormat(buffer))
			file = new BDVFile(buffer);
		else if (BVRFile::verifyFormat(buffer))
			file = new BVRFile(buffer);
		
		if (file && file->valid) {
			return file; //return new BRDBoard(file);
		}
	}
	return nullptr;
}

int BoardView::LoadFile(const filesystem::path &filepath) {
	m_lastFileOpenWasInvalid = true;
	m_validBoard             = false;
	if (!filepath.empty()) {
		// clean up the previous file.
		if (m_file && m_board) {
			for (auto &p : m_board->Components()) {
				if (p->hull) free(p->hull);
			}
			m_pinHighlighted.clear();
			m_partHighlighted.clear();
			m_annotations.Close();
			m_board->OutlinePoints().clear();
			if constexpr (! std::is_same<obv_shared_ptr<Component>, std::shared_ptr<Component> >::value) {
				delete m_file.get();
				delete m_board.get();
				m_file = nullptr;
				m_board = nullptr;
			} else {
				m_board->Nets().clear();
				m_board->Pins().clear();
				m_board->Components().clear();
			}
		}

		SetLastFileOpenName(filepath.string());
		std::vector<char> buffer = file_as_buffer(filepath);
		if (!buffer.empty()) {
			BRDFile *file = nullptr;

			if (check_fileext(filepath, ".fz")) { // Since it is encrypted we cannot use the below logic. Trust the ext.
				file = new FZFile(buffer, FZKey);
			} else if (check_fileext(filepath, ".bom") || check_fileext(filepath, ".asc"))
				file = new ASCFile(buffer, filepath);
			else if (ADFile::verifyFormat(buffer))
				file = new ADFile(buffer);
			else if (CADFile::verifyFormat(buffer))
				file = new CADFile(buffer);
			//			else if (CAMCADFile::verifyFormat(buffer))
			//	file = new CAMCADFile(buffer);
			else if (check_fileext(filepath, ".cst"))
				file = new CSTFile(buffer);
			else if (BRDFile::verifyFormat(buffer))
				file = new BRDFile(buffer);
			else if (BRD2File::verifyFormat(buffer))
				file = new BRD2File(buffer);
			else if (BDVFile::verifyFormat(buffer))
				file = new BDVFile(buffer);
			else if (BVRFile::verifyFormat(buffer))
				file = new BVRFile(buffer);

			if (file && file->valid) {
				SetFile(obv_shared_ptr<BRDFile>(file));
				fhistory.Prepend_save(filepath.string());
				history_file_has_changed = 1; // used by main to know when to update the window title
				boardMinMaxDone          = false;
				m_rotation               = 0;
				m_current_side           = 0;
				EPCCheck(); // check to see we don't have a flipped board outline

				m_annotations.SetFilename(filepath.string());
				m_annotations.Load();

				auto conffilepath = filepath;
				conffilepath.replace_extension("conf");
				backgroundImage.loadFromConfig(conffilepath);

				/*
				 * Set pins to a known lower size, they get resized
				 * in DrawParts() when the component is analysed
				 */
				for (auto &p : m_board->Pins()) {
					//					auto p      = pin.get();
					p->diameter = 7;
				}

				CenterView();
				m_lastFileOpenWasInvalid = false;
				m_validBoard             = true;

			} else {
				m_validBoard = false;
				delete file;
			}
		}
	} else {
		return 1;
	}

	return 0;
}

void BoardView::SetFZKey(const char *keytext) {

	if (keytext) {
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

void RA(const char *t, int w) {
	ImVec2 s = ImGui::CalcTextSize(t);
	s.x      = w - s.x;
	ImGui::Dummy(s);
	ImGui::SameLine();
	ImGui::Text("%s", t);
}

void u32tof4(uint32_t c, float *f) {
	f[0] = (c & 0xff) / 0xff;
	c >>= 8;
	f[1] = (c & 0xff) / 0xff;
	c >>= 8;
	f[2] = (c & 0xff) / 0xff;
	c >>= 8;
	f[3] = (c & 0xff) / 0xff;
}
void BoardView::ColorPreferencesItem(
    const char *label, int label_width, const char *butlabel, const char *conflabel, int var_width, uint32_t *c) {
	std::string desc_id = "##" + std::string(label);
	char buf[20];
	snprintf(buf, sizeof(buf), "%08X", byte4swap(*c));
	RA(label, label_width);
	ImGui::SameLine();
	ImGui::ColorButton(desc_id.c_str(), ImColor(*c));
	ImGui::SameLine();
	ImGui::PushItemWidth(var_width);
	if (ImGui::InputText(
	        butlabel, buf, sizeof(buf), ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsHexadecimal, nullptr, buf)) {
		*c = byte4swap(strtoll(buf, NULL, 16));
		snprintf(buf, sizeof(buf), "0x%08x", byte4swap(*c));
		obvconfig.WriteStr(conflabel, buf);
	}
	ImGui::PopItemWidth();
}


void BoardView::SaveAllColors(void) {
	cfg_color_visit_each(m_colors, [this](const char * cfg, uint32_t c, uint32_t, uint32_t) {
							 obvconfig.WriteHex(cfg, byte4swap(c));
						 });
}

void BoardView::ColorPreferences(void) {
	bool dummy = true;
	//	ImGui::PushStyleColor(ImGuiCol_PopupBg, 0xffe0e0e0);
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), 0, ImVec2(0.5f, 0.5f));;
	if (ImGui::BeginPopupModal("Colour Preferences", &dummy, ImGuiWindowFlags_AlwaysAutoResize)) {

		if (m_showColorPreferences) {
			m_showColorPreferences = false;
			m_tooltips_enabled     = false;
		}

		ImGui::Dummy(ImVec2(1, DPI(5)));
		RA("Base theme", DPI(200));
		ImGui::SameLine();
		{
			int tc        = 0;
			const char *v = obvconfig.ParseStr("colorTheme", "default");
			if (strcmp(v, "dark") == 0) {
				tc = 1;
			}
			if (ImGui::RadioButton("Light", &tc, 0)) {
				obvconfig.WriteStr("colorTheme", "light");
				ThemeSetStyle("light");
				SaveAllColors();
				m_needsRedraw = true;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Dark", &tc, 1)) {
				obvconfig.WriteStr("colorTheme", "dark");
				ThemeSetStyle("dark");
				SaveAllColors();
				m_needsRedraw = true;
			}
		}
		ImGui::Dummy(ImVec2(1, DPI(5)));

		ColorPreferencesItem("Background", DPI(200), "##Background", "backgroundColor", DPI(150), &m_colors.backgroundColor);
		ColorPreferencesItem("Board fill", DPI(200), "##Boardfill", "boardFillColor", DPI(150), &m_colors.boardFillColor);
		ColorPreferencesItem(
		    "Board outline", DPI(200), "##BoardOutline", "boardOutlineColor", DPI(150), &m_colors.boardOutlineColor);

		ImGui::Dummy(ImVec2(1, DPI(10)));
		ImGui::Text("Parts");
		ImGui::Separator();
		ColorPreferencesItem("Outline", DPI(200), "##PartOutline", "partOutlineColor", DPI(150), &m_colors.partOutlineColor);
		ColorPreferencesItem("Hull", DPI(200), "##PartHull", "partHullColor", DPI(150), &m_colors.partHullColor);
		ColorPreferencesItem("Fill", DPI(200), "##PartFill", "partFillColor", DPI(150), &m_colors.partFillColor);
		ColorPreferencesItem(
		    "Selected", DPI(200), "##PartSelected", "partHighlightedColor", DPI(150), &m_colors.partHighlightedColor);
		ColorPreferencesItem("Fill (selected)",
		                     DPI(200),
		                     "##PartFillSelected",
		                     "partHighlightedFillColor",
		                     DPI(150),
		                     &m_colors.partHighlightedFillColor);
		ColorPreferencesItem("Text", DPI(200), "##PartText", "partTextColor", DPI(150), &m_colors.partTextColor);
		ColorPreferencesItem("Text background",
		                     DPI(200),
		                     "##PartTextBackground",
		                     "partTextBackgroundColor",
		                     DPI(150),
		                     &m_colors.partTextBackgroundColor);

		ImGui::Dummy(ImVec2(1, DPI(10)));
		ImGui::Text("Pins");
		ImGui::Separator();
		ColorPreferencesItem("Default", DPI(200), "##PinDefault", "pinDefaultColor", DPI(150), &m_colors.pinDefaultColor);
		ColorPreferencesItem(
		    "Default text", DPI(200), "##PinDefaultText", "pinDefaultTextColor", DPI(150), &m_colors.pinDefaultTextColor);
		ColorPreferencesItem("Ground", DPI(200), "##PinGround", "pinGroundColor", DPI(150), &m_colors.pinGroundColor);
		ColorPreferencesItem("NC", DPI(200), "##PinNC", "pinNotConnectedColor", DPI(150), &m_colors.pinNotConnectedColor);
		ColorPreferencesItem("Test pad", DPI(200), "##PinTP", "pinTestPadColor", DPI(150), &m_colors.pinTestPadColor);
		ColorPreferencesItem("Test pad fill", DPI(200), "##PinTPF", "pinTestPadFillColor", DPI(150), &m_colors.pinTestPadFillColor);
		ColorPreferencesItem("A1/1 Pad", DPI(200), "##PinA1", "pinA1PadColor", DPI(150), &m_colors.pinA1PadColor);

		ColorPreferencesItem("Selected", DPI(200), "##PinSelectedColor", "pinSelectedColor", DPI(150), &m_colors.pinSelectedColor);
		ColorPreferencesItem(
		    "Selected fill", DPI(200), "##PinSelectedFillColor", "pinSelectedFillColor", DPI(150), &m_colors.pinSelectedFillColor);
		ColorPreferencesItem(
		    "Selected text", DPI(200), "##PinSelectedTextColor", "pinSelectedTextColor", DPI(150), &m_colors.pinSelectedTextColor);

		ColorPreferencesItem("Same Net", DPI(200), "##PinSameNetColor", "pinSameNetColor", DPI(150), &m_colors.pinSameNetColor);
		ColorPreferencesItem(
		    "SameNet fill", DPI(200), "##PinSameNetFillColor", "pinSameNetFillColor", DPI(150), &m_colors.pinSameNetFillColor);
		ColorPreferencesItem(
		    "SameNet text", DPI(200), "##PinSameNetTextColor", "pinSameNetTextColor", DPI(150), &m_colors.pinSameNetTextColor);

		ColorPreferencesItem("Halo", DPI(200), "##PinHalo", "pinHaloColor", DPI(150), &m_colors.pinHaloColor);
		ColorPreferencesItem("Net web strands", DPI(200), "##NetWebStrands", "pinNetWebColor", DPI(150), &m_colors.pinNetWebColor);
		ColorPreferencesItem(
		    "Net web (otherside)", DPI(200), "##NetWebOSStrands", "pinNetWebOSColor", DPI(150), &m_colors.pinNetWebOSColor);

		ImGui::Dummy(ImVec2(1, DPI(10)));
		ImGui::Text("Annotations");
		ImGui::Separator();
		ColorPreferencesItem("Box", DPI(200), "##AnnBox", "annotationBoxColor", DPI(150), &m_colors.annotationBoxColor);
		ColorPreferencesItem("Stalk", DPI(200), "##AnnStalk", "annotationStalkColor", DPI(150), &m_colors.annotationStalkColor);
		ColorPreferencesItem(
		    "Popup text", DPI(200), "##AnnPopupText", "annotationPopupTextColor", DPI(150), &m_colors.annotationPopupTextColor);
		ColorPreferencesItem("Popup background",
		                     DPI(200),
		                     "##AnnPopupBackground",
		                     "annotationPopupBackgroundColor",
		                     DPI(150),
		                     &m_colors.annotationPopupBackgroundColor);

		ImGui::Dummy(ImVec2(1, DPI(10)));
		ImGui::Text("Masks");
		ImGui::Separator();
		ColorPreferencesItem("Pins", DPI(200), "##MaskPins", "selectedMaskPins", DPI(150), &m_colors.selectedMaskPins);
		ColorPreferencesItem("Parts", DPI(200), "##MaskParts", "selectedMaskParts", DPI(150), &m_colors.selectedMaskParts);
		ColorPreferencesItem("Outline", DPI(200), "##MaskOutline", "selectedMaskOutline", DPI(150), &m_colors.selectedMaskOutline);
		ColorPreferencesItem("Boost (Pins)", DPI(200), "##BoostPins", "orMaskPins", DPI(150), &m_colors.orMaskPins);
		ColorPreferencesItem("Boost (Parts)", DPI(200), "##BoostParts", "orMaskParts", DPI(150), &m_colors.orMaskParts);
		ColorPreferencesItem("Boost (Outline)", DPI(200), "##BoostOutline", "orMaskOutline", DPI(150), &m_colors.orMaskOutline);

		ImGui::Separator();

		if (ImGui::Button("Done") || keybindings.isPressed("CloseDialog")) {
			m_tooltips_enabled = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (!dummy) {
		m_tooltips_enabled = true;
	}
	//	ImGui::PopStyleColor();
}

void BoardView::Preferences(void) {
	bool dummy = true;
	ImGuiStyle &style = ImGui::GetStyle();
	//	ImGui::PushStyleColor(ImGuiCol_PopupBg, 0xffe0e0e0);
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), 0, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Preferences", &dummy, ImGuiWindowFlags_AlwaysAutoResize)) {
		int t;
		if (m_showPreferences) {
			m_showPreferences  = false;
			m_tooltips_enabled = false;
		}

		t = obvconfig.ParseInt("windowX", 1100);
		RA("Window Width", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##windowX", &t)) {
			if (t > 400) obvconfig.WriteInt("windowX", t);
		}

		t = obvconfig.ParseInt("windowY", 700);
		RA("Window Height", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##windowY", &t)) {
			if (t > 320) obvconfig.WriteInt("windowY", t);
		}

		const char *oldFont = obvconfig.ParseStr("fontName", "");
		std::vector<char> newFont(oldFont, oldFont + strlen(oldFont) + 1); // Copy string data + '\0' char
		if (newFont.size() < 256)                                          // reserve space for new font name
			newFont.resize(256, '\0');                                     // Max font name length is 255 characters
		RA("Font name", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputText("##fontName", newFont.data(), newFont.size())) {
			obvconfig.WriteStr("fontName", newFont.data());
		}

		t = obvconfig.ParseInt("fontSize", 20);
		RA("Font size", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##fontSize", &t)) {
			if ((t >= 8) && (t < 60)) obvconfig.WriteInt("fontSize", t);
		}

		t = obvconfig.ParseInt("dpi", 100);
		RA("Screen DPI", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##dpi", &t)) {
			if ((t > 25) && (t < 600)) obvconfig.WriteInt("dpi", t);
		}

		if (ImGui::Checkbox("Start in Fullscreen", &startFullscreen)) {
			obvconfig.WriteBool("startFullscreen", startFullscreen);
		}

		ImGui::PushStyleColor(ImGuiCol_Text, 0xff4040ff);
		ImGui::Text("(Program restart is required to properly apply font and DPI changes)");
		ImGui::PopStyleColor();

		ImGui::Separator();

		RA("Board fill spacing", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##boardFillSpacing", &boardFillSpacing)) {
			obvconfig.WriteInt("boardFillSpacing", boardFillSpacing);
		}

		t = zoomFactor * 10;
		RA("Zoom step", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##zoomStep", &t)) {
			obvconfig.WriteFloat("zoomFactor", t / 10);
		}

		RA("Zoom modifier", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##zoomModifier", &zoomModifier)) {
			obvconfig.WriteInt("zoomModifier", zoomModifier);
		}

		RA("Panning step", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##panningStep", &panFactor)) {
			obvconfig.WriteInt("panFactor", panFactor);
		}

		RA("Pan modifier", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##panModifier", &panModifier)) {
			obvconfig.WriteInt("panModifier", panModifier);
		}

		ImGui::Dummy(ImVec2(1, DPI(5)));
		RA("Flip Mode", DPI(200));
		ImGui::SameLine();
		{
			if (ImGui::RadioButton("Viewport", &flipMode, 0)) {
				obvconfig.WriteInt("flipMode", flipMode);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Mouse", &flipMode, 1)) {
				obvconfig.WriteInt("flipMode", flipMode);
			}
		}
		ImGui::Dummy(ImVec2(1, DPI(5)));

		RA("Annotation flag size", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##annotationBoxSize", &annotationBoxSize)) {
			obvconfig.WriteInt("annotationBoxSize", annotationBoxSize);
		}

		RA("Annotation flag offset", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##annotationBoxOffset", &annotationBoxOffset)) {
			obvconfig.WriteInt("annotationBoxOffset", annotationBoxOffset);
		}
		RA("Net web thickness", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##netWebThickness", &netWebThickness)) {
			obvconfig.WriteInt("netWebThickness", netWebThickness);
		}
		ImGui::Separator();

		RA("Pin-1/A1 count threshold", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputInt("##pinA1threshold", &pinA1threshold)) {
			if (pinA1threshold < 1) pinA1threshold = 1;
			obvconfig.WriteInt("pinA1threshold", pinA1threshold);
		}

		if (ImGui::Checkbox("Pin select masks", &pinSelectMasks)) {
			obvconfig.WriteBool("pinSelectMasks", pinSelectMasks);
		}

		if (ImGui::Checkbox("Pin Halo", &pinHalo)) {
			obvconfig.WriteBool("pinHalo", pinHalo);
		}
		RA("Halo diameter", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputFloat("##haloDiameter", &pinHaloDiameter)) {
			obvconfig.WriteFloat("pinHaloDiameter", pinHaloDiameter);
		}

		RA("Halo thickness", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputFloat("##haloThickness", &pinHaloThickness)) {
			obvconfig.WriteFloat("pinHaloThickness", pinHaloThickness);
		}

		RA("Info Panel Zoom", DPI(200));
		ImGui::SameLine();
		if (ImGui::InputFloat("##partZoomScaleOutFactor", &partZoomScaleOutFactor)) {
			if (partZoomScaleOutFactor < 1.1) partZoomScaleOutFactor = 1.1;
			obvconfig.WriteFloat("partZoomScaleOutFactor", partZoomScaleOutFactor);
		}

		if (ImGui::Checkbox("Center/Zoom Search Results", &m_centerZoomSearchResults)) {
			obvconfig.WriteBool("centerZoomSearchResults", m_centerZoomSearchResults);
		}

		if (ImGui::Checkbox("Show Info Panel", &showInfoPanel)) {
			obvconfig.WriteBool("showInfoPanel", showInfoPanel);
		}

		if (ImGui::Checkbox("Show net web", &showNetWeb)) {
			obvconfig.WriteBool("showNetWeb", showNetWeb);
		}

		if (ImGui::Checkbox("slowCPU", &slowCPU)) {
			obvconfig.WriteBool("slowCPU", slowCPU);
			style.AntiAliasedLines = !slowCPU;
			style.AntiAliasedFill  = !slowCPU;
		}

		ImGui::SameLine();
		if (ImGui::Checkbox("Show FPS", &showFPS)) {
			obvconfig.WriteBool("showFPS", showFPS);
		}

		if (ImGui::Checkbox("Fill Parts", &fillParts)) {
			obvconfig.WriteBool("fillParts", fillParts);
		}

		ImGui::SameLine();
		if (ImGui::Checkbox("Fill Board", &boardFill)) {
			obvconfig.WriteBool("boardFill", boardFill);
		}

		ImGui::Separator();
		{
			char keybuf[1024];
			int i;
			ImGui::Text("FZ Key");
			ImGui::SameLine();
			for (i = 0; i < 44; i++) {
				sprintf(keybuf + (i * 12),
				        "0x%08lx%s",
				        (long unsigned int)FZKey[i],
				        (i != 43) ? ((i + 1) % 4 ? "  " : "\r\n")
				                  : ""); // yes, a nested inline-if-else. add \r\n after every 4 values, except if the last
			}
			if (ImGui::InputTextMultiline(
			        "##fzkey", keybuf, sizeof(keybuf), ImVec2(DPI(450), ImGui::GetTextLineHeight() * 12.5), 0, NULL, keybuf)) {

				// Strip the line breaks out.
				char *c = keybuf;
				while (*c) {
					if ((*c == '\r') || (*c == '\n')) *c = ' ';
					c++;
				}
				obvconfig.WriteStr("FZKey", keybuf);
				SetFZKey(keybuf);
			}
		}

		ImGui::Separator();

		if (ImGui::Button("Done") || keybindings.isPressed("CloseDialog")) {
			ImGui::CloseCurrentPopup();
			m_tooltips_enabled = true;
		}
		ImGui::EndPopup();
	}

	if (!dummy) {
		m_tooltips_enabled = true;
	}
	//	ImGui::PopStyleColor();
}

void BoardView::HelpAbout(void) {
	bool dummy = true;
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), 0, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("About", &dummy, ImGuiWindowFlags_AlwaysAutoResize)) {
		if (m_showHelpAbout) {
			m_showHelpAbout    = false;
			m_tooltips_enabled = false;
		}
		ImGui::Text("%s %s", OBV_NAME, OBV_VERSION);
		ImGui::Text("Build %s %s", OBV_BUILD, __TIMESTAMP__);
		ImGui::Text(OBV_URL);
		if (ImGui::Button("Close") || keybindings.isPressed("CloseDialog")) {
			m_tooltips_enabled = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::Dummy(ImVec2(1, DPI(10)));
		ImGui::Text("License info");
		ImGui::Separator();
		ImGui::TextWrapped(OBV_LICENSE_TEXT);

		ImGui::EndPopup();
	}

	if (!dummy) {
		m_tooltips_enabled = true;
	}
}

void BoardView::HelpControls(void) {
	bool dummy = true;
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), 0, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(DPI(540), 0.0f));
	if (ImGui::BeginPopupModal("Controls", &dummy)) {
		if (m_showHelpControls) {
			m_showHelpControls = false;
			m_tooltips_enabled = false;
		}
		ImGui::Text("KEYBOARD CONTROLS");
		ImGui::SameLine();

		if (ImGui::Button("Close") || keybindings.isPressed("CloseDialog")) {
			m_tooltips_enabled = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::Separator();

		ImGui::Columns(2);
		ImGui::PushItemWidth(-1);
		ImGui::Text("Load file");
		ImGui::Text("Quit");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("Pan up");
		ImGui::Text("Pan down");
		ImGui::Text("Pan left");
		ImGui::Text("Pan right");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("Toggle information panel");
		ImGui::Text("Search for component/Net");
		ImGui::Text("Display component list");
		ImGui::Text("Display net list");
		ImGui::Text("Clear all highlighted items");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("Mirror board");
		ImGui::Text("Flip board");
		ImGui::Text(" ");

		ImGui::Text("Zoom in");
		ImGui::Text("Zoom out");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("Reset zoom and center");
		ImGui::Text("Rotate clockwise");
		ImGui::Text("Rotate anticlockwise");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("Toggle Pin Blanking");

		/*
		 * NEXT COLUMN
		 */
		ImGui::NextColumn();
		ImGui::Text("CTRL-o");
		ImGui::Text("CTRL-q");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("w, numpad-up, arrow-up");
		ImGui::Text("s, numpad-down, arrow-down");
		ImGui::Text("a, numpad-left, arrow-left");
		ImGui::Text("d, numpad-right, arrow-right");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("i");
		ImGui::Text("CTRL-f, /");
		ImGui::Text("k");
		ImGui::Text("l");
		ImGui::Text("ESC");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("m");
		ImGui::Text("Space bar");
		ImGui::Text("(+shift to hold position)");
		ImGui::Spacing();

		ImGui::Text("+, numpad+");
		ImGui::Text("-, numpad-");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("x, numpad-5");
		ImGui::Text("'.' numpad '.'");
		ImGui::Text("',' numpad-0");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("p");

		ImGui::Columns(1);
		ImGui::Separator();

		ImGui::Text("MOUSE CONTROLS");
		ImGui::Separator();
		ImGui::Columns(2);
		ImGui::Text("Highlight pins on network");
		ImGui::Text("Move board");
		ImGui::Text("Zoom (CTRL for finer steps)");
		ImGui::Text("Flip board");
		ImGui::Text("Annotations menu");

		ImGui::NextColumn();
		ImGui::Text("Click (on pin)");
		ImGui::Text("Click and drag");
		ImGui::Text("Scroll");
		ImGui::Text("Middle click");
		ImGui::Text("Right click");
		ImGui::Columns(1);

		ImGui::EndPopup();
	}
	if (!dummy) {
		m_tooltips_enabled = true;
	}
}

void BoardView::ShowInfoPane(void) {
	ImGuiIO &io = ImGui::GetIO();
	ImVec2 ds   = io.DisplaySize;

	if (!showInfoPanel) return;

	if (m_info_surface.x < DPIF(100)) {
		//	fprintf(stderr,"Too small (%f), set to (%f)\n", width, DPIF(100));
		m_info_surface.x  = DPIF(100) + 1;
		m_board_surface.x = ds.x - m_info_surface.x;
	}

	m_info_surface.y = m_board_surface.y;

	/*
	 * Originally the dialog was to follow the cursor but it proved to be an overkill
	 * to try adjust things to keep it within the bounds of the window so as to not
	 * lose the buttons.
	 *
	 * Now it's kept at a fixed point.
	 */
	ImGui::SetNextWindowPos(ImVec2(ds.x - m_info_surface.x, m_menu_height));
	ImGui::SetNextWindowSize(m_info_surface);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::Begin("Info Panel",
	             NULL,
	             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
	                 ImGuiWindowFlags_NoSavedSettings);

	if ((m_dragging_token == 0) && (io.MousePos.x > m_board_surface.x) && (io.MousePos.x < (m_board_surface.x + DPIF(12.0f)))) {
		ImDrawList *draw = ImGui::GetWindowDrawList();
		draw->AddRectFilled(ImVec2(m_board_surface.x, m_menu_height),
		                    ImVec2(m_board_surface.x + DPIF(12.0f), m_board_surface.y + m_menu_height),
		                    ImColor(0x88888888));
		//			DrawHex( draw, io.MousePos, DPIF(10.0f), ImColor(0xFF0000FF) );
	}

	if (ImGui::IsMouseDragging(0)) {
		if ((m_dragging_token == 0) && (io.MouseClickedPos[0].x > m_board_surface.x) &&
		    (io.MouseClickedPos[0].x < (m_board_surface.x + DPIF(20.0f))))
			m_dragging_token = 2; // own it.
		if (m_dragging_token == 2) {
			ImVec2 delta = ImGui::GetMouseDragDelta();
			if ((abs(delta.x) > 500) || (abs(delta.y) > 500)) {
				delta.x = 0;
				delta.y = 0;
			} // If the delta values are crazy just drop them (happens when panning
			// off screen). 500 arbritary chosen
			ImGui::ResetMouseDragDelta();
			m_board_surface.x += delta.x;
			m_info_surface.x = ds.x - m_board_surface.x;
			if (m_board_surface.x < ds.x * 0.66) {
				m_board_surface.x = ds.x * 0.66;
				m_info_surface.x  = ds.x - m_board_surface.x;
			}
			if (delta.x > 0) m_needsRedraw = true;
		}
	} else {
		if (m_dragging_token == 2) {
			obvconfig.WriteInt("infoPanelWidth", m_info_surface.x);
		}
		m_dragging_token = 0;
	}

	if (m_board) {
		ImGui::Columns(2);
		ImGui::Text("Pins: %zu", m_board->Pins().size());
		ImGui::Text("Parts: %zu", m_board->Components().size());
		ImGui::NextColumn();
		ImGui::Text("Nets: %zu", m_board->Nets().size());
		ImGui::TextWrapped("Size: %0.2f x %0.2f\"", m_boardWidth / 1000.0f, m_boardHeight / 1000.0f);
		ImGui::Columns(1);
		ImGui::Separator();
		if (ImGui::Checkbox("Zoom on selected net", &infoPanelCenterZoomNets)) {
			obvconfig.WriteBool("infoPanelCenterZoomNets", infoPanelCenterZoomNets);
		}
		if (ImGui::Checkbox("Select all parts on net", &infoPanelSelectPartsOnNet)) {
			obvconfig.WriteBool("infoPanelSelectPartsOnNet", infoPanelSelectPartsOnNet);
		}
	} else {
		ImGui::Text("No board currently loaded.");
	}

	if (m_partHighlighted.size()) {
		ImGui::Separator();
		ImGui::TextUnformatted((std::to_string(m_partHighlighted.size()) + " parts selected").c_str());

		for (auto part : m_partHighlighted) {

			ImGui::Text(" ");

			bool center_comp = false;
			if (ImGui::SmallButton(part->name.c_str())) {
				center_comp = true;
			}
			ImGui::SameLine();
			{
				char bn[128];
				snprintf(bn, sizeof(bn), "Z##%s", part->name.c_str());
				if (ImGui::SmallButton(bn)) {
					center_comp = true;
				}
			}

			if (center_comp) {
				if (!BoardElementIsVisible(part)) FlipBoard();
				if (part->centerpoint.x && part->centerpoint.y) {
					ImVec2 psz;

					/*
					 * Check to see if we need to zoom BACK a bit to fit the part in to view
					 */
					psz = abs(part->outline[2] - part->outline[0]) * m_scale / m_board_surface;
					if ((psz.x > 1) || (psz.y > 1)) {
						if (psz.x > psz.y) {
							m_scale /= (partZoomScaleOutFactor * psz.x);
						} else {
							m_scale /= (partZoomScaleOutFactor * psz.y);
						}
					}

					SetTarget(part->centerpoint);
				} else {
					SetTarget(part->pins[0]->position);
				}
				m_needsRedraw = 1;
			}

			ImGui::SameLine();
			ImGui::Text("%zu Pin(s)", part->pins.size());
			ImGui::SameLine();
			{
				char name_and_id[128];
				snprintf(name_and_id, sizeof(name_and_id), "Copy##%s", part->name.c_str());
				if (ImGui::SmallButton(name_and_id)) {
					// std::string speed is no concern here, since button action is not in UI rendering loop
					std::string to_copy = part->name;
					if (part->mfgcode.size()) {
						to_copy += " " + part->mfgcode;
					}
					for (const auto &pin : part->pins) {
						to_copy += "\n" + pin->name + " " + pin->net->name;
					}
					ImGui::SetClipboardText(to_copy.c_str());
				}
			}
			if (part->mfgcode.size()) ImGui::TextWrapped("%s", part->mfgcode.c_str());

			/*
			 * Generate the pin# and net table
			 */
			ImGui::PushItemWidth(-1);
			std::string str = std::string("##") + part->name;
			ImVec2 listSize;
			int pc = part->pins.size();
			if (pc > 20) pc = 20;
			listSize = ImVec2(m_info_surface.x - DPIF(50), pc * ImGui::GetFontSize() * 1.45);
			if (ImGui::ListBoxHeader(str.c_str(), listSize)) { //, ImVec2(m_board_surface.x/3 -5, m_board_surface.y/2));
				for (auto pin : part->pins) {
					char ss[1024];
					snprintf(ss, sizeof(ss), "%4s  %s", pin->name.c_str(), pin->net->name.c_str());
					if (ImGui::Selectable(ss, (pin == m_pinSelected))) {
						ClearAllHighlights();

						if ((pin->type == Pin::kPinTypeNotConnected) || (pin->type == Pin::kPinTypeUnkown) || (pin->net->is_ground)) {
							m_partHighlighted.push_back(pin->component);
							// do nothing for now
							//
						} else {
							m_pinSelected = pin;
							//for (auto p : m_partHighlighted) {
							//	pin->component->visualmode = pin->component->CVMNormal;
							//};
							pin->component->visualmode = pin->component->CVMNormal;
							m_partHighlighted.push_back(pin->component);
							CenterZoomNet(pin->net->name);
						}
						m_needsRedraw = true;
					}
					ImGui::PushStyleColor(ImGuiCol_Border, 0xffeeeeee);
					ImGui::Separator();
					ImGui::PopStyleColor();
				}
				ImGui::ListBoxFooter();
			}
			ImGui::PopItemWidth();

		} // for each part in the list
		ImGui::Text(" ");
		ImGui::Separator();
		ImGui::Separator();
		ImGui::Separator();
	}

	ImGui::End();
	ImGui::PopStyleVar();
}

void BoardView::ContextMenu(void) {
	bool dummy                       = true;
	static char contextbuf[10240]    = "";
	static char contextbufnew[10240] = "";
	static std::string pin, partn, net;
	double tx, ty;

	ImVec2 pos = ScreenToCoord(m_showContextMenuPos.x, m_showContextMenuPos.y);

	/*
	 * So we don't have dozens of near-same-spot annotation points, we truncate
	 * back to integer levels, which still gives us 1-thou precision
	 */
	tx = trunc(pos.x);
	ty = trunc(pos.y);

	/*
	 * Originally the dialog was to follow the cursor but it proved to be an overkill
	 * to try adjust things to keep it within the bounds of the window so as to not
	 * lose the buttons.
	 *
	 * Now it's kept at a fixed point.
	 */
	ImGui::SetNextWindowPos(ImVec2(DPIF(50), DPIF(50)));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	if (ImGui::BeginPopupModal("Annotations", &dummy, ImGuiWindowFlags_AlwaysAutoResize)) {

		if (m_showContextMenu) {
			contextbuf[0]      = 0;
			m_showContextMenu  = false;
			m_tooltips_enabled = false;
			//			m_parent_occluded = true;
			for (auto &ann : m_annotations.annotations) ann.hovered = false;
		}

		/*
		 * For the new annotation possibility, we need to detect our various
		 * attributes that we can bind to, such as pin, net, part
		 */
		{
			/*
			 * we're going to go through all the possible items we can annotate at this position and offer them
			 */

			float min_dist = m_pinDiameter * 1.0f;

			/*
			 * find the closest pin, starting at no more than
			 * 1 radius away
			 */
			min_dist *= min_dist; // all distance squared
			Pin *selection = nullptr;
			for (auto &pin : m_board->Pins()) {
				if (BoardElementIsVisible(pin->component)) {
					float dx   = pin->position.x - pos.x;
					float dy   = pin->position.y - pos.y;
					float dist = dx * dx + dy * dy;
					if (dist < min_dist) {
						selection = pin.get();
						min_dist  = dist;
					}
				}
			}

			/*
			 * If there was a pin selected, we can extract net/part off it
			 */

			if (selection != nullptr) {
				pin   = selection->name;
				partn = selection->component->name;
				net   = selection->net->name;
			} else {

				/*
				 * ELSE if we didn't get a pin selected, we can still
				 * check for a part.
				 *
				 * There is a problem where we can be on two parts
				 * but haven't decided what to do in such a situation
				 */

				for (auto &part : m_board->Components()) {
					int hit = 0;
					//					auto p_part = part.get();

					if (!BoardElementIsVisible(part)) continue;

					// Work out if the point is inside the hull
					{
						int i, j, n;
						outline_pt *poly;

						n    = 4;
						poly = part->outline;

						for (i = 0, j = n - 1; i < n; j = i++) {
							if (((poly[i].y > pos.y) != (poly[j].y > pos.y)) &&
							    (pos.x < (poly[j].x - poly[i].x) * (pos.y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x))
								hit = !hit;
						}
					} // hull test
					if (hit) {
						partn = part->name;

						ImGui::SameLine();
					}

				} // for each part
			}

			{

				/*
				 * For existing annotations
				 */
				if (m_annotation_clicked_id >= 0) {
					if (m_annotationedit_retain || (m_annotation_clicked_id >= 0)) {
						Annotation ann = m_annotations.annotations[m_annotation_clicked_id];
						if (!m_annotationedit_retain) {
							snprintf(contextbuf, sizeof(contextbuf), "%s", ann.note.c_str());
							m_annotationedit_retain = true;
							m_annotationnew_retain  = false;
						}

						ImGui::Text("%c(%0.0f,%0.0f) %s, %s%c%s%c",
						            m_current_side ? 'B' : 'T',
						            tx,
						            ty,
						            ann.net.c_str(),
						            ann.part.c_str(),
						            ann.part.size() && ann.pin.size() ? '[' : ' ',
						            ann.pin.c_str(),
						            ann.part.size() && ann.pin.size() ? ']' : ' ');
						ImGui::InputTextMultiline("##annotationedit",
						                          contextbuf,
						                          sizeof(contextbuf),
						                          ImVec2(DPI(600), ImGui::GetTextLineHeight() * 8),
						                          0,
						                          NULL,
						                          contextbuf);

						if (ImGui::Button("Update##1") || keybindings.isPressed("Validate")) {
							m_annotationedit_retain = false;
							m_annotations.Update(m_annotations.annotations[m_annotation_clicked_id].id, contextbuf);
							m_annotations.GenerateList();
							m_needsRedraw      = true;
							m_tooltips_enabled = true;
							// m_parent_occluded = false;
							ImGui::CloseCurrentPopup();
						}
						ImGui::SameLine();
						if (ImGui::Button("Cancel##1")) {
							ImGui::CloseCurrentPopup();
							m_annotationnew_retain = false;
							m_tooltips_enabled     = true;
							// m_parent_occluded = false;
						}
						ImGui::Separator();
					}
				}

				/*
				 * For generating a new annotation
				 */
				if ((m_annotation_clicked_id < 0) && (m_annotationnew_retain == false)) {
					ImGui::Text("%c(%0.0f,%0.0f) %s %s%c%s%c",
					            m_current_side ? 'B' : 'T',
					            tx,
					            ty,
					            net.c_str(),
					            partn.c_str(),
					            partn.empty() || pin.empty() ? ' ' : '[',
					            pin.c_str(),
					            partn.empty() || pin.empty() ? ' ' : ']');
				}
				if ((m_annotation_clicked_id < 0) || ImGui::Button("Add New##1") || m_annotationnew_retain) {
					if (m_annotationnew_retain == false) {
						contextbufnew[0]        = 0;
						m_annotationnew_retain  = true;
						m_annotation_clicked_id = -1;
						m_annotationedit_retain = false;
					}

					ImGui::Text("Create new annotation for: %c(%0.0f,%0.0f) %s %s%c%s%c",
					            m_current_side ? 'B' : 'T',
					            tx,
					            ty,
					            net.c_str(),
					            partn.c_str(),
					            partn.empty() || pin.empty() ? ' ' : '[',
					            pin.c_str(),
					            partn.empty() || pin.empty() ? ' ' : ']');
					ImGui::Spacing();
					ImGui::InputTextMultiline("New##annotationnew",
					                          contextbufnew,
					                          sizeof(contextbufnew),
					                          ImVec2(DPI(600), ImGui::GetTextLineHeight() * 8),
					                          0,
					                          NULL,
					                          contextbufnew);

					if (ImGui::Button("Apply##1") || keybindings.isPressed("Validate")) {
						m_tooltips_enabled     = true;
						m_annotationnew_retain = false;
						if (debug) fprintf(stderr, "DATA:'%s'\n\n", contextbufnew);

						m_annotations.Add(m_current_side, tx, ty, net.c_str(), partn.c_str(), pin.c_str(), contextbufnew);
						m_annotations.GenerateList();
						m_needsRedraw = true;

						ImGui::CloseCurrentPopup();
					}
					/*
					ImGui::SameLine();
					if (ImGui::Button("Cancel")) {
					    ImGui::CloseCurrentPopup();
					    m_tooltips_enabled     = true;
					    m_annotationnew_retain = false;
					}
					*/
				} else {
					ImGui::SameLine();
				}

				if ((m_annotation_clicked_id >= 0) && (ImGui::Button("Remove"))) {
					m_annotations.Remove(m_annotations.annotations[m_annotation_clicked_id].id);
					m_annotations.GenerateList();
					m_needsRedraw = true;
					// m_parent_occluded = false;
					ImGui::CloseCurrentPopup();
				}
			}

			// the position.
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel##2") || keybindings.isPressed("CloseDialog")) {
			m_annotationnew_retain  = false;
			m_annotationedit_retain = false;
			m_tooltips_enabled      = true;
			// m_parent_occluded = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
		//	m_tooltips_enabled = true;
	}
	ImGui::PopStyleVar();

	/** if the dialog was closed by using the (X) icon **/
	if (!dummy) {
		m_annotationnew_retain  = false;
		m_annotationedit_retain = false;
		m_tooltips_enabled      = true;
		// m_parent_occluded = false;
	}
}

std::pair<SharedVector<Component>, SharedVector<Net>> BoardView::SearchPartsAndNets(const char *search, int limit) {
	SharedVector<Component> parts;
	SharedVector<Net> nets;
	if (m_searchComponents) parts = searcher.parts(search, limit);
	if (m_searchNets) nets = searcher.nets(search, limit);
	return {parts, nets};
}

const char *getcname(const std::string &name) {
	return name.c_str();
}

template <class T>
const char *getcname(const T &t) {
	return t->name.c_str();
}

template <class T>
void BoardView::ShowSearchResults(std::vector<T> results, char *search, int &limit, void (BoardView::*onSelect)(const char *)) {
	for (auto &r : results) {
		const char *cname = getcname(r);
		if (ImGui::Selectable(cname, false)) {
			(this->*onSelect)(cname);
			snprintf(search, 128, "%s", cname);
			limit = 0;
		}
		limit--;
	}
}

void BoardView::SearchColumnGenerate(const std::string &title,
                                     std::pair<SharedVector<Component>, SharedVector<Net>> results,
                                     char *search,
                                     int limit) {
	if (ImGui::ListBoxHeader(title.c_str())) {

		if (m_searchComponents) {
			if (results.first.empty() && (!m_searchNets || results.second.empty())) { // show suggestions only if there is no result at all
				auto s = scparts.suggest(search);
				if (s.size() > 0) {
					ImGui::Text("Did you mean...");
					ShowSearchResults(s, search, limit, &BoardView::FindComponent);
				}
			} else
				ShowSearchResults(results.first, search, limit, &BoardView::FindComponent);
		}

		if (m_searchNets) {
			if (results.second.empty() && (!m_searchComponents || results.first.empty())) {
				auto s = scnets.suggest(search);
				if (s.size() > 0) {
					ImGui::Text("Did you mean...");
					ShowSearchResults(s, search, limit, &BoardView::FindNet);
				}
			} else
				ShowSearchResults(results.second, search, limit, &BoardView::FindNet);
		}

		ImGui::ListBoxFooter();
	}
}

void BoardView::SearchComponent(void) {
	bool dummy = true;

	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x/2, DPI(100)), 0, ImVec2(0.5f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	if (ImGui::BeginPopupModal("Search for Component / Network",
	                           &dummy,
	                           ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
		//		char cs[128];
		const char *first_button[] = {m_search[0], m_search[1], m_search[2]};

		if (m_showSearch) {
			m_showSearch       = false;
			m_tooltips_enabled = false;
			//			fprintf(stderr, "Tooltips disabled\n");
		}

		// Column 1, implied.
		//
		//
		if (ImGui::Button("Search")) {
			// FindComponent(first_button);
			m_tooltips_enabled = true;
			SearchCompound(first_button[0]);
			SearchCompoundNoClear(first_button[1]);
			SearchCompoundNoClear(first_button[2]);
			CenterZoomSearchResults();
			ImGui::CloseCurrentPopup();
		} // search button

		ImGui::SameLine();
		if (ImGui::Button("Reset")) {
			FindComponent("");
			for (int i = 0; i < 3; i++) m_search[i][0] = '\0';
		} // reset button

		ImGui::SameLine();
		if (ImGui::Button("Exit") || keybindings.isPressed("CloseDialog")) {
			FindComponent("");
			for (int i = 0; i < 3; i++) m_search[i][0] = '\0';
			m_tooltips_enabled = true;
			ImGui::CloseCurrentPopup();
		} // exit button

		ImGui::SameLine();
		//		ImGui::Dummy(ImVec2(DPI(200), 1));
		//		ImGui::SameLine();
		ImGui::PushItemWidth(-1);
		ImGui::Text("ENTER: Search, ESC: Exit, TAB: next field");

		ImGui::Separator();
		ImGui::Checkbox("Components", &m_searchComponents);

		ImGui::SameLine();
		ImGui::Checkbox("Nets", &m_searchNets);

		{
			ImGui::SameLine();
			ImGui::Text(" Search mode: ");
			ImGui::SameLine();
			if (ImGui::RadioButton("Substring", searcher.isMode(SearchMode::Sub))) {
				searcher.setMode(SearchMode::Sub);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Prefix", searcher.isMode(SearchMode::Prefix))) {
				searcher.setMode(SearchMode::Prefix);
			}
			ImGui::SameLine();
			ImGui::PushItemWidth(-1);
			if (ImGui::RadioButton("Whole", searcher.isMode(SearchMode::Whole))) {
				searcher.setMode(SearchMode::Whole);
			}
			ImGui::PopItemWidth();
		}

		ImGui::Separator();

		ImGui::Columns(3);

		for (int i = 1; i <= 3; i++) {
			std::string istr        = std::to_string(i);
			std::string title       = "Item #" + istr;
			std::string searchLabel = "##search" + istr;
			ImGui::Text("%s", title.c_str());

			ImGui::PushItemWidth(-1);

			bool searching  = m_search[i - 1][0] != '\0';                        // Text typed in the search box
			auto results    = SearchPartsAndNets(m_search[i - 1], 30);           // Perform the search for both nets and parts
			bool hasResults = !results.first.empty() || !results.second.empty(); // We found some nets or some parts

			if (searching && !hasResults) ImGui::PushStyleColor(ImGuiCol_FrameBg, 0xFF6666FF);
			auto ret = ImGui::InputText(searchLabel.c_str(),
			                            m_search[i - 1],
			                            128,
			                            ImGuiInputTextFlags_CharsNoBlank | (m_search[0] ? ImGuiInputTextFlags_AutoSelectAll : 0));
			if (searching && !hasResults) ImGui::PopStyleColor();

			if (ret) SearchCompound(m_search[i - 1]);

			ImGui::PopItemWidth();

			if (i == 1 && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
				ImGui::SetKeyboardFocusHere(-1);
			} // set keyboard focus

			ImGui::PushItemWidth(-1);
			if (searching) SearchColumnGenerate("##SC" + istr, results, m_search[i - 1], 30);
			ImGui::PopItemWidth();
			if (i == 1)
				ImGui::PushItemWidth(DPI(500));
			else if (i == 2)
				ImGui::PopItemWidth();

			ImGui::NextColumn();
		}

		ImGui::PopItemWidth();

		ImGui::Columns(1); // reset back to single column mode
		ImGui::Separator();

		// Enter and Esc close the search:
		if (keybindings.isPressed("Accept")) {
			// SearchCompound(first_button);
			// SearchCompoundNoClear(first_button2);
			// SearchCompoundNoClear(first_button3);
			SearchCompound(m_search[0]);
			SearchCompoundNoClear(m_search[1]);
			SearchCompoundNoClear(m_search[2]);
			CenterZoomSearchResults();
			ImGui::CloseCurrentPopup();
			m_tooltips_enabled = true;
		} // response to keyboard ENTER

		ImGui::EndPopup();
	}
	ImGui::PopStyleVar();
	if (!dummy) {
		m_tooltips_enabled = true;
	}
}

void BoardView::ClearAllHighlights(void) {
	m_pinSelected = nullptr;
	FindNet("");
	FindComponent("");
	m_search[0][0]     = '\0';
	m_search[1][0]     = '\0';
	m_search[2][0]     = '\0';
	m_needsRedraw      = true;
	m_tooltips_enabled = true;
	if (m_board) {
		for (auto part : m_board->Components()) part->visualmode = part->CVMNormal;
	}
	m_partHighlighted.clear();
	m_pinHighlighted.clear();
}

/** UPDATE Logic region
 *
 *
 *
 *
 */
void BoardView::Update() {
	bool open_file = false;
	// ImGuiIO &io = ImGui::GetIO();
	char *preset_filename = nullptr;
	ImGuiIO &io           = ImGui::GetIO();

	/**
	 * ** FIXME
	 * This should be handled in the keyboard section, not here
	 */
	if (keybindings.isPressed("Open")) {
		open_file = true;
		// the dialog will likely eat our WM_KEYUP message for CTRL and O:
		io.KeysDown[SDL_SCANCODE_RCTRL] = false;
		io.KeysDown[SDL_SCANCODE_LCTRL] = false;
		io.KeysDown[SDL_SCANCODE_O]     = false;
	}

	if (keybindings.isPressed("Quit")) {
		m_wantsQuit = true;
	}

	if (ImGui::BeginMainMenuBar()) {
		m_menu_height = ImGui::GetWindowHeight();

		/*
		 * Create these dialogs, but they're not actually
		 * visible until they're called up by the respective
		 * flags.
		 */
		SearchComponent();
		HelpControls();
		HelpAbout();
		ColorPreferences();
		Preferences();
		ContextMenu();

		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open", keybindings.getKeyNames("Open").c_str())) {
				open_file = true;
			}

			/// Generate file history - PLD20160618-2028
			ImGui::Separator();
			{
				int i;
				for (i = 0; i < fhistory.count; i++) {
					if (ImGui::MenuItem(fhistory.Trim_filename(fhistory.history[i], 2))) {
						open_file       = true;
						preset_filename = fhistory.history[i];
					}
				}
			}
			ImGui::Separator();

			if (ImGui::MenuItem("Part/Net Search", keybindings.getKeyNames("Search").c_str())) {
				if (m_validBoard) m_showSearch = true;
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Program Preferences")) {
				m_showPreferences = true;
			}

			if (ImGui::MenuItem("Colour Preferences")) {
				m_showColorPreferences = true;
			}

			keyboardPreferences.menuItem();
			if (tcl_keyboardPreferences) {
				tcl_keyboardPreferences->menuItem("TCL shortcuts");
			}
			backgroundImagePreferences.menuItem();

			ImGui::Separator();

			if (ImGui::MenuItem("Quit", keybindings.getKeyNames("Quit").c_str())) {
				m_wantsQuit = true;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View")) {
			if (ImGui::MenuItem("Flip board", keybindings.getKeyNames("Flip").c_str())) {
				FlipBoard();
			}
			if (ImGui::MenuItem("Rotate CW", keybindings.getKeyNames("RotateCW").c_str())) {
				Rotate(1);
			}
			if (ImGui::MenuItem("Rotate CCW", keybindings.getKeyNames("RotateCCW").c_str())) {
				Rotate(-1);
			}
			if (ImGui::MenuItem("Reset View", keybindings.getKeyNames("Center").c_str())) {
				CenterView();
			}
			if (ImGui::MenuItem("Mirror Board", keybindings.getKeyNames("Mirror").c_str())) {
				Mirror();
			}

			if (ImGui::MenuItem("Toggle Pin Display", keybindings.getKeyNames("TogglePins").c_str())) {
				showPins ^= 1;
				m_needsRedraw = true;
			}

			if (ImGui::MenuItem("Show Info Panel", keybindings.getKeyNames("InfoPanel").c_str())) {
				showInfoPanel ^= 1;
				obvconfig.WriteBool("showInfoPanel", showInfoPanel ? true : false);
				m_needsRedraw = true;
			}

			ImGui::Separator();
			if (ImGui::Checkbox("Show FPS", &showFPS)) {
				obvconfig.WriteBool("showFPS", showFPS);
				m_needsRedraw = true;
			}

			if (ImGui::Checkbox("Show Position", &showPosition)) {
				obvconfig.WriteBool("showPosition", showPosition);
				m_needsRedraw = true;
			}

			if (ImGui::Checkbox("Net web", &showNetWeb)) {
				obvconfig.WriteBool("showNetWeb", showNetWeb);
				m_needsRedraw = true;
			}

			if (ImGui::Checkbox("Annotations", &showAnnotations)) {
				obvconfig.WriteBool("showAnnotations", showAnnotations);
				m_needsRedraw = true;
			}

			if (ImGui::Checkbox("Board fill", &boardFill)) {
				obvconfig.WriteBool("boardFill", boardFill);
				m_needsRedraw = true;
			}

			if (ImGui::Checkbox("Part fill", &fillParts)) {
				obvconfig.WriteBool("fillParts", fillParts);
				m_needsRedraw = true;
			}

			if (ImGui::Checkbox("Background image", &backgroundImage.enabled)) {
				obvconfig.WriteBool("showBackgroundImage", backgroundImage.enabled);
				m_needsRedraw = true;
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Info Panel", keybindings.getKeyNames("InfoPanel").c_str())) {
				showInfoPanel = !showInfoPanel;
			}
			if (ImGui::MenuItem("Net List", keybindings.getKeyNames("NetList").c_str())) {
				m_showNetList = !m_showNetList;
			}
			if (ImGui::MenuItem("Part List", keybindings.getKeyNames("PartList").c_str())) {
				m_showPartList = !m_showPartList;
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help##1")) {
			if (ImGui::MenuItem("Controls")) {
				m_showHelpControls = true;
			}
			if (ImGui::MenuItem("About")) {
				m_showHelpAbout = true;
			}
			ImGui::EndMenu();
		}

		ImGui::SameLine();
		ImGui::Dummy(ImVec2(DPI(10), 1));
		ImGui::SameLine();
		if (ImGui::Checkbox("Annotations", &showAnnotations)) {
			obvconfig.WriteBool("showAnnotations", showAnnotations);
			m_needsRedraw = true;
		}

		ImGui::SameLine();
		if (ImGui::Checkbox("Netweb", &showNetWeb)) {
			obvconfig.WriteBool("showNetWeb", showNetWeb);
			m_needsRedraw = true;
		}

		ImGui::SameLine();
		{
			if (ImGui::Checkbox("Pins", &showPins)) {
				obvconfig.WriteBool("showPins", showPins);
				m_needsRedraw = true;
			}
		}

		ImGui::SameLine();
		if (ImGui::Checkbox("Image", &backgroundImage.enabled)) {
			obvconfig.WriteBool("showBackgroundImage", backgroundImage.enabled);
			m_needsRedraw = true;
		}

		ImGui::SameLine();
		ImGui::Dummy(ImVec2(DPI(40), 1));

		ImGui::SameLine();
		if (ImGui::Button(" - ")) {
			Zoom(m_board_surface.x / 2, m_board_surface.y / 2, -zoomFactor);
		}
		ImGui::SameLine();
		if (ImGui::Button(" + ")) {
			Zoom(m_board_surface.x / 2, m_board_surface.y / 2, zoomFactor);
		}
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(DPI(20), 1));
		ImGui::SameLine();
		if (ImGui::Button("-")) {
			Zoom(m_board_surface.x / 2, m_board_surface.y / 2, -zoomFactor / zoomModifier);
		}
		ImGui::SameLine();
		if (ImGui::Button("+")) {
			Zoom(m_board_surface.x / 2, m_board_surface.y / 2, zoomFactor / zoomModifier);
		}

		ImGui::SameLine();
		ImGui::Dummy(ImVec2(DPI(20), 1));
		ImGui::SameLine();
		if (ImGui::Button(" < ")) {
			Rotate(-1);
		}

		ImGui::SameLine();
		if (ImGui::Button(" ^ ")) {
			FlipBoard();
		}

		ImGui::SameLine();
		if (ImGui::Button(" > ")) {
			Rotate(1);
		}

		ImGui::SameLine();
		if (ImGui::Button("X")) {
			CenterView();
		}

		ImGui::SameLine();
		if (ImGui::Button("CLEAR")) {
			ClearAllHighlights();
		}

		/*
		ImGui::SameLine();
		ImVec2 tz = ImGui::CalcTextSize("Search             ");
		ImGui::PushItemWidth(-tz.x);
		ImGui::Text("Search");
		ImGui::SameLine();
		ImGui::PushItemWidth(-DPI(1));
		if (ImGui::InputText("##ansearch", m_search, 128, 0)) {
		}
		ImGui::PopItemWidth();
		*/

		if (m_showContextMenu && m_file && showAnnotations) {
			ImGui::OpenPopup("Annotations");
		}

		if (m_showHelpAbout) {
			ImGui::OpenPopup("About");
		}
		if (m_showHelpControls) {
			ImGui::OpenPopup("Controls");
		}

		if (m_showColorPreferences) {
			ImGui::OpenPopup("Colour Preferences");
		}

		if (m_showPreferences) {
			ImGui::OpenPopup("Preferences");
		}

		keyboardPreferences.render();
		backgroundImagePreferences.render();

		if (tcl_keyboardPreferences) {
			tcl_keyboardPreferences->render("TCL shortcuts");
		}
		
		if (m_showSearch && m_file) {
			ImGui::OpenPopup("Search for Component / Network");
		}

		if (ImGui::BeginPopupModal("Error opening file")) {
			ImGui::Text("There was an error opening the file: %s", m_lastFileOpenName.c_str());
			if (check_fileext(m_lastFileOpenName, ".fz")) {
				int i;
				ImGui::Separator();
				ImGui::Text("FZKey:");
				for (i = 0; i < 44; i++) {
					// snprintf(s, sizeof(s), "%08x %08x %08x %08x", FZKey[i], FZKey[i + 1], FZKey[i + 2], FZKey[i + 3]);
					ImGui::Text("%08x %08x %08x %08x", FZKey[i], FZKey[i + 1], FZKey[i + 2], FZKey[i + 3]);
					//					ImGui::Text(s);
					i += 3;
				}
			}
			// TODO: error details? -- would need the loader to say what's wrong.
			if (ImGui::Button("OK")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		if (m_lastFileOpenWasInvalid) {
			ImGui::OpenPopup("Error opening file");
			m_lastFileOpenWasInvalid = false;
		}
		ImGui::EndMainMenuBar();
	}

	if (open_file) {
		filesystem::path filepath;

		if (preset_filename) {
			filepath        = filesystem::u8path(preset_filename);
			preset_filename = NULL;
		} else {
			filepath = show_file_picker(true);

			ImGuiIO &io           = ImGui::GetIO();
			io.MouseDown[0]       = false;
			io.MouseClicked[0]    = false;
			io.MouseClickedPos[0] = ImVec2(0, 0);
		}

		if (!filepath.empty()) {
			LoadFile(filepath);
		}
	}

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
	                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

	ImGuiWindowFlags draw_surface_flags = flags | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollWithMouse;

	/*
	 * Status footer
	 */
	m_status_height = (DPIF(10.0f) + ImGui::GetFontSize());
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(DPIF(4.0f), DPIF(3.0f)));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y - m_status_height));
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, m_status_height));
	ImGui::Begin("status", nullptr, flags | ImGuiWindowFlags_NoFocusOnAppearing);
	if (m_file && m_board && m_pinSelected) {
		auto pin = m_pinSelected;
		ImGui::Text("Part: %s   Pin: %s   Net: %s   Probe: %d   (%s.)",
		            pin->component->name.c_str(),
		            pin->name.c_str(),
		            pin->net->name.c_str(),
		            pin->net->number,
		            pin->component->mount_type_str().c_str());
	} else {
		ImVec2 spos = ImGui::GetMousePos();
		ImVec2 pos  = ScreenToCoord(spos);
		if (showFPS == true) {
			ImGui::Text("FPS: %0.0f ", ImGui::GetIO().Framerate);
			ImGui::SameLine();
		}

		if (debug) {
			ImGui::Text("AnnID:%d ", m_annotation_clicked_id);
			ImGui::SameLine();
		}

		if (showPosition == true) {
			ImGui::Text("Position: %0.3f\", %0.3f\" (%0.2f, %0.2fmm)", pos.x / 1000, pos.y / 1000, pos.x * 0.0254, pos.y * 0.0254);
			ImGui::SameLine();
		}
		{
			if (m_validBoard) {
				ImVec2 s = ImGui::CalcTextSize(fhistory.history[0]);
				ImGui::SameLine(ImGui::GetWindowWidth() - s.x - 20);
				ImGui::Text("%s", fhistory.history[0]);
				ImGui::SameLine();
			}
		}
		ImGui::Text(" ");
	}
	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();

	if (!showInfoPanel) {
		m_board_surface = ImVec2(io.DisplaySize.x, io.DisplaySize.y - (m_status_height + m_menu_height));
	} else {
		m_board_surface = ImVec2(io.DisplaySize.x - m_info_surface.x, io.DisplaySize.y - (m_status_height + m_menu_height));
	}
	/*
	 * Drawing surface, where the actual PCB/board is plotted out
	 */
	ImGui::SetNextWindowPos(ImVec2(0, m_menu_height));

	//if (io.DisplaySize.x != m_lastWidth || io.DisplaySize.y != m_lastHeight) {
	if (m_board_surface.x != m_lastWidth || m_board_surface.y != m_lastHeight) {
	//		m_lastWidth   = io.DisplaySize.x;
		//		m_lastHeight  = io.DisplaySize.y;
		m_lastWidth   = m_board_surface.x;
		m_lastHeight  = m_board_surface.y;
		m_needsRedraw = true;
	}
	ImGui::SetNextWindowSize(m_board_surface);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, m_colors.backgroundColor);

	ImGui::Begin("surface", nullptr, draw_surface_flags);
	HandleInput();

	if (m_validBoard) {
		backgroundImage.render(*ImGui::GetWindowDrawList(),
			CoordToScreen(backgroundImage.x0(), backgroundImage.y0()),
			CoordToScreen(backgroundImage.x1(), backgroundImage.y1()),
			m_rotation);
		DrawBoard();

		if (m_draw_both_sides) {
			m_current_side ^= 1;
			//m_dx = -m_dx;
			if (m_current_side) {
				//m_mx += 10000;
			} else {
				//m_mx -= 10000;
			}
			//m_y_offset = m_boardHeight;
			m_dy -= m_boardHeight;
			m_dx = - m_dx; //-= m_boardWidth;
			
			m_needsRedraw = true;
			DrawBoard();
			
			m_y_offset = 0;
			m_current_side ^= 1;
			m_dy += m_boardHeight;
			//m_dx += m_boardWidth;
			m_dx = -m_dx;
			if (m_current_side) {
				//m_mx -= 10000;
			} else {
				//m_mx += 10000;
			}
		}
	}
	ImGui::End();

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();

	// Overlay
	RenderOverlay();

	ImGui::PopStyleVar();

} // main menu bar

void BoardView::Zoom(float osd_x, float osd_y, float zoom) {
	ImVec2 target;
	ImVec2 coord;
	ImGuiIO &io = ImGui::GetIO();

	if (io.KeyCtrl) zoom /= zoomModifier;

	target.x = osd_x;
	target.y = osd_y;
	coord    = ScreenToCoord(target.x, target.y);

	// Adjust the scale of the whole view, then get the new coordinates ( as
	// CoordToScreen utilises m_scale )
	m_scale        = m_scale * powf(2.0f, zoom);
	ImVec2 dtarget = CoordToScreen(coord.x, coord.y);

	ImVec2 td = ScreenToCoord(target.x - dtarget.x, target.y - dtarget.y, 0);
	m_dx += td.x;
	m_dy += td.y;
	m_needsRedraw = true;
}

void BoardView::Pan(int direction, int amount) {
	ImGuiIO &io = ImGui::GetIO();
#define DIR_UP 1
#define DIR_DOWN 2
#define DIR_LEFT 3
#define DIR_RIGHT 4

	amount = amount / m_scale;

	if (io.KeyCtrl) amount /= panModifier;

	switch (direction) {
		case DIR_UP: amount = -amount;
			// fall through
	    case DIR_DOWN:
			if ((m_current_side) && (m_rotation % 2)) amount = -amount;
			switch (m_rotation) {
				case 0: m_dy += amount; break;
				case 1: m_dx -= amount; break;
				case 2: m_dy -= amount; break;
				case 3: m_dx += amount; break;
			}
			break;
		case DIR_LEFT: amount = -amount;
			// fall through
	    case DIR_RIGHT:
			if ((m_current_side) && ((m_rotation % 2) == 0)) amount = -amount;
			switch (m_rotation) {
				case 0: m_dx -= amount; break;
				case 1: m_dy -= amount; break;
				case 2: m_dx += amount; break;
				case 3: m_dy += amount; break;
			}
			break;
	}

	m_draggingLastFrame = true;
	m_needsRedraw       = true;
}

/*
 * This HandleInput() function isn't actually a generic input handler
 * it's meant specifically for the board draw surface only.  The inputs
 * for menus is handled within the menu generation itself.
 */
void BoardView::HandleInput() {
	if (!m_board || (!m_file)) return;

	const ImGuiIO &io = ImGui::GetIO();

	if (ImGui::IsWindowHovered()) {
		if (ImGui::IsMouseDragging(0)) {
			if ((m_dragging_token == 0) && (io.MouseClickedPos[0].x < m_board_surface.x)) m_dragging_token = 1; // own it.
			if (m_dragging_token == 1) {
				//		   if ((io.MouseClickedPos[0].x < m_info_surface.x)) {
				ImVec2 delta = ImGui::GetMouseDragDelta();
				if ((abs(delta.x) > 500) || (abs(delta.y) > 500)) {
					delta.x = 0;
					delta.y = 0;
				} // If the delta values are crazy just drop them (happens when panning
				// off screen). 500 arbritary chosen
				ImGui::ResetMouseDragDelta();

				if (! (m_tcl_drag = m_tcl->handle_mouse_drag(io.MouseClickedPos[0], delta, m_tcl_drag))) {
					ImVec2 td = ScreenToCoord(delta.x, delta.y, 0);

					m_dx += td.x;
					m_dy += td.y;
				}
				m_draggingLastFrame = true;
				m_needsRedraw       = true;
			}
		} else if (m_dragging_token >= 0) {
			m_dragging_token = 0;
			m_tcl_drag = false;
			
			if (m_lastFileOpenWasInvalid == false) {
				// Conext menu

				if (!m_lastFileOpenWasInvalid && m_file && m_board && ImGui::IsMouseClicked(1)) {
					if (showAnnotations) {
						// Build context menu here, for annotations and inspection
						//
						ImVec2 spos = ImGui::GetMousePos();
						if (AnnotationIsHovered()) m_annotation_clicked_id = m_annotation_last_hovered;

						m_annotationedit_retain = false;
						m_showContextMenu       = true;
						m_showContextMenuPos    = spos;
						m_tooltips_enabled      = false;
						m_needsRedraw           = true;
						if (debug) fprintf(stderr, "context click request at (%f %f)\n", spos.x, spos.y);
					}

					// Flip the board with the middle click
				} else if (!m_lastFileOpenWasInvalid && m_file && m_board && ImGui::IsMouseReleased(2)) {
					FlipBoard();

					// Else, click to select pin
				} else if (!m_lastFileOpenWasInvalid && m_file && m_board && ImGui::IsMouseReleased(0) && !m_draggingLastFrame && !m_tcl->handle_mouse_click(io.MouseClickedPos[0])) {
					ImVec2 spos = ImGui::GetMousePos();
					ImVec2 pos  = ScreenToCoord(spos.x, spos.y);

					m_needsRedraw = true;

					// threshold to within a pin's diameter of the pin center
					// float min_dist = m_pinDiameter * 1.0f;
					float min_dist = m_pinDiameter / 2.0f;
					min_dist *= min_dist; // all distance squared
					obv_shared_ptr<Pin> selection = nullptr;
					for (auto &pin : m_board->Pins()) {
						if (BoardElementIsVisible(pin->component)) {
							float dx   = pin->position.x - pos.x;
							float dy   = pin->position.y - pos.y;
							float dist = dx * dx + dy * dy;
							if ((dist < (pin->diameter * pin->diameter)) && (dist < min_dist)) {
								selection = pin;
								min_dist  = dist;
							}
						}
					}

					m_pinSelected = selection;
					if (m_pinSelected) {
						if (!io.KeyCtrl) {
							for (auto p : m_board->Components()) {
								p->visualmode = p->CVMNormal;
							}
							m_partHighlighted.resize(0);
							m_pinHighlighted.resize(0);
						}
						m_pinSelected->component->visualmode = m_pinSelected->component->CVMSelected;
						m_partHighlighted.push_back(m_pinSelected->component);
					}

					if (m_pinSelected == nullptr) {
						bool any_hits = false;

						for (auto &part : m_board->Components()) {
							int hit = 0;
							//							auto p_part = part.get();

							if (!BoardElementIsVisible(part)) continue;

							// Work out if the point is inside the hull
							{
								int i, j, n;
								outline_pt *poly;

								n    = 4;
								poly = part->outline;

								for (i = 0, j = n - 1; i < n; j = i++) {
									if (((poly[i].y > pos.y) != (poly[j].y > pos.y)) &&
									    (pos.x <
									     (poly[j].x - poly[i].x) * (pos.y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x))
										hit ^= 1;
								}
							}

							if (hit) {
								any_hits = true;

								bool partInList = contains(part, m_partHighlighted);

								/*
								 * If the CTRL key isn't held down, then we have to
								 * flush any existing highlighted parts
								 */
								if (io.KeyCtrl) {
									if (!partInList) {
										m_partHighlighted.push_back(part);
										part->visualmode = part->CVMSelected;
									} else {
										remove(part, m_partHighlighted);
										part->visualmode = part->CVMNormal;
									}

								} else {
									for (auto p : m_board->Components()) {
										p->visualmode = p->CVMNormal;
									}
									m_partHighlighted.resize(0);
									m_pinHighlighted.resize(0);
									if (!partInList) {
										m_partHighlighted.push_back(part);
										part->visualmode = part->CVMSelected;
									}
								}
								m_tcl->component_select_event();

								
								/*
								 * If this part has a non-selected visual mode (normal)
								 * AND it's not in the existing part list, then add it
								 */
								/*
								if (part->visualmode == part->CVMNormal) {
								    if (!partInList) {
								        m_partHighlighted.push_back(p_part);
								    }
								}

								part->visualmode++;
								part->visualmode %= part->CVMModeCount;

								if (part->visualmode == part->CVMNormal) {
								    remove(*part, m_partHighlighted);
								}
								*/
							} // if hit
						}     // for each part on the board

						/*
						 * If we aren't holding down CTRL and we click to a
						 * non pin, non part area, then we clear everything
						 */
						if ((!any_hits) && (!io.KeyCtrl)) {
							for (auto &part : m_board->Components()) {
								//								auto p        = part.get();
								part->visualmode = part->CVMNormal;
							}
							m_partHighlighted.clear();
						}

					} // if a pin wasn't selected

				} else {
					if (!m_showContextMenu) {
						if (AnnotationIsHovered()) {
							m_needsRedraw        = true;
							AnnotationWasHovered = true;
						} else {
							AnnotationWasHovered = false;
							m_needsRedraw        = true;
						}
					}
				}

				m_draggingLastFrame = false;
			}

			// Zoom:
			float mwheel = io.MouseWheel;
			if (mwheel != 0.0f) {
				mwheel *= zoomFactor;

				if (! m_tcl->handle_mouse_wheel(io.MousePos.x, io.MousePos.y, io.MouseWheel)) {
					Zoom(io.MousePos.x, io.MousePos.y, mwheel);
				} else {
					m_needsRedraw = true;
				}
			}
		}
	}

	if ((!io.WantCaptureKeyboard)) {

		if (keybindings.isPressed("Mirror")) {
			Mirror();
			CenterView();
			m_needsRedraw = true;

		} else if (keybindings.isPressed("RotateCW")) {
			// Rotate board: R and period rotate clockwise; comma rotates
			// counter-clockwise
			Rotate(1);

		} else if (keybindings.isPressed("RotateCCW")) {
			Rotate(-1);

		} else if (keybindings.isPressed("ZoomIn")) {
			Zoom(m_board_surface.x / 2, m_board_surface.y / 2, zoomFactor);

		} else if (keybindings.isPressed("ZoomOut")) {
			Zoom(m_board_surface.x / 2, m_board_surface.y / 2, -zoomFactor);

		} else if (keybindings.isPressed("PanDown")) {
			Pan(DIR_DOWN, panFactor);

		} else if (keybindings.isPressed("PanUp")) {
			Pan(DIR_UP, panFactor);

		} else if (keybindings.isPressed("PanLeft")) {
			Pan(DIR_LEFT, panFactor);

		} else if (keybindings.isPressed("PanRight")) {
			Pan(DIR_RIGHT, panFactor);

		} else if (keybindings.isPressed("Center")) {
			// Center and reset zoom
			CenterView();

		} else if (keybindings.isPressed("Quit")) {
			// quit OFBV
			m_wantsQuit = true;

		} else if (keybindings.isPressed("InfoPanel")) {
			showInfoPanel = !showInfoPanel;
			obvconfig.WriteBool("showInfoPanel", showInfoPanel ? true : false);

		} else if (keybindings.isPressed("NetList")) {
			// Show Net List
			m_showNetList = m_showNetList ? false : true;

		} else if (keybindings.isPressed("PartList")) {
			// Show Part List
			m_showPartList = m_showPartList ? false : true;

		} else if (keybindings.isPressed("Flip")) {
			// Flip board:
			FlipBoard();

		} else if (keybindings.isPressed("TogglePins")) {
			showPins ^= 1;
			m_needsRedraw = true;

		} else if (keybindings.isPressed("Search")) {
			if (m_validBoard) {
				m_showSearch  = true;
				m_needsRedraw = true;
			}

		} else if (keybindings.isPressed("Clear")) {
			ClearAllHighlights();

		} else if (keybindings.isPressed("Fullscreen")) {
			//SDL_SetWindowFullscreen(m_sdl_window, m_is_fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
			if (m_is_fullscreen) {
				SDL_RestoreWindow(m_sdl_window);
			} else {
				SDL_MaximizeWindow(m_sdl_window);
			}
			m_is_fullscreen = !m_is_fullscreen;
		} else {
			bool handled = false;
			if (tcl_keybindings) {
				for (auto && ii : m_tcl->keybind_targets()) {
					if (tcl_keybindings->isPressed(ii)) {
						m_tcl->keypress(ii);
						handled = true;
					}
				}
			}

			if (! handled) {
			/*
			 * Do what ever is required for unhandled key presses.
			 */
			}
		}
	}
}

/* END UPDATE REGION */

/** Overlay and Windows
 *
 *
 */
void BoardView::ShowNetList(bool *p_open) {
	static NetList netList(bind(&BoardView::FindNet, this, _1));
	netList.Draw("Net List", p_open, m_board.get());
}

void BoardView::ShowPartList(bool *p_open) {
	static PartList partList(bind(&BoardView::FindComponent, this, _1));
	partList.Draw("Part List", p_open, m_board.get());
}

void BoardView::RenderOverlay() {

	ShowInfoPane();

	// Listing of Net elements
	if (m_showNetList) {
		ShowNetList(&m_showNetList);
	}
	if (m_showPartList) {
		ShowPartList(&m_showPartList);
	}
}

/** End overlay & windows region **/

/**
 * Drawing region
 *
 *
 *
 */

void BoardView::CenterZoomNet(string netname) {
	ImVec2 view = m_board_surface;
	ImVec2 min, max;
	int i = 0;

	if (!infoPanelCenterZoomNets) return;

	min.x = min.y = FLT_MAX;
	max.x = max.y = FLT_MIN;

	for (auto &pin : m_board->Pins()) {
		if (pin->net->name == netname) {
			auto p = pin->position;
			if (p.x < min.x) min.x = p.x;
			if (p.y < min.y) min.y = p.y;
			if (p.x > max.x) max.x = p.x;
			if (p.y > max.y) max.y = p.y;

			if ((infoPanelSelectPartsOnNet) && (pin->type != Pin::kPinTypeTestPad)) {
				if (!contains(pin->component, m_partHighlighted)) {
					pin->component->visualmode = pin->component->CVMSelected;
					m_partHighlighted.push_back(pin->component);
				}
			}
		}
	}

	// Bounds check!
	if ((min.x == FLT_MAX) || (min.y == FLT_MAX) || (max.x == FLT_MIN) || (max.y == FLT_MIN)) return;

	if (debug) fprintf(stderr, "CenterzoomNet: bbox[%d]: %0.1f %0.1f - %0.1f %0.1f\n", i, min.x, min.y, max.x, max.y);

	float dx = (max.x - min.x);
	float dy = (max.y - min.y);
	float sx = dx > 0 ? view.x / dx : FLT_MAX;
	float sy = dy > 0 ? view.y / dy : FLT_MAX;

	m_scale = sx < sy ? sx : sy;
	if (m_scale == FLT_MAX) m_scale = m_scale_floor;
	m_scale /= partZoomScaleOutFactor;
	if (m_scale < m_scale_floor) m_scale = m_scale_floor;

	/*
	float dx = (max.x - min.x);
	float dy = (max.y - min.y);
	float sx = dx > 0 ? view.x / dx : 1.0f;
	float sy = dy > 0 ? view.y / dy : 1.0f;

	//  m_rotation = 0;
	m_scale = sx < sy ? sx : sy;
	m_scale /= partZoomScaleOutFactor;
	if (m_scale < m_scale_floor) m_scale = m_scale_floor;
	*/

	m_dx = (max.x - min.x) / 2 + min.x;
	m_dy = (max.y - min.y) / 2 + min.y;
	SetTarget(m_dx, m_dy);
	m_needsRedraw = true;
}

void BoardView::CenterZoomSearchResults(void) {
	// ImVec2 view = ImGui::GetIO().DisplaySize;
	ImVec2 view = m_board_surface;
	ImVec2 min, max;
	int i = 0;

	if (!showPins) showPins = true; // Louis Rossmann UI failure fix.

	if (!m_centerZoomSearchResults) return;

	min.x = min.y = FLT_MAX;
	max.x = max.y = FLT_MIN;

	for (auto &pp : m_pinHighlighted) {
		auto &p = pp->position;
		if (p.x < min.x) min.x = p.x;
		if (p.y < min.y) min.y = p.y;
		if (p.x > max.x) max.x = p.x;
		if (p.y > max.y) max.y = p.y;
		i++;
	}

	for (auto &pp : m_partHighlighted) {
		for (auto &pn : pp->pins) {
			auto &p = pn->position;
			if (p.x < min.x) min.x = p.x;
			if (p.y < min.y) min.y = p.y;
			if (p.x > max.x) max.x = p.x;
			if (p.y > max.y) max.y = p.y;
			i++;
		}
	}

	// Bounds check!
	if ((min.x == FLT_MAX) || (min.y == FLT_MAX) || (max.x == FLT_MIN) || (max.y == FLT_MIN)) return;

	if (debug) fprintf(stderr, "CenterzoomSearchResults: bbox[%d]: %0.1f %0.1f - %0.1f %0.1f\n", i, min.x, min.y, max.x, max.y);
	// fprintf(stderr, "CenterzoomSearchResults: bbox[%d]: %0.1f %0.1f - %0.1f %0.1f\n", i, min.x, min.y, max.x, max.y);

	float dx = (max.x - min.x);
	float dy = (max.y - min.y);
	float sx = dx > 0 ? view.x / dx : FLT_MAX;
	float sy = dy > 0 ? view.y / dy : FLT_MAX;

	m_scale = sx < sy ? sx : sy;
	if (m_scale == FLT_MAX) m_scale = m_scale_floor;
	m_scale /= partZoomScaleOutFactor;
	if (m_scale < m_scale_floor) m_scale = m_scale_floor;

	m_dx = (max.x - min.x) / 2 + min.x;
	m_dy = (max.y - min.y) / 2 + min.y;
	SetTarget(m_dx, m_dy);
	m_needsRedraw = true;
}

/*
 * EPC = External Pin Count; finds pins which are not contained within
 * the outline and flips the board outline if required, as it seems some
 * brd2 files are coming with a y-flipped outline
 */
int BoardView::EPCCheck(void) {
	int epc[2] = {0, 0};
	int side;
	auto &outline = m_board->OutlinePoints();
	ImVec2 min, max;

	// find the orthagonal bounding box
	// probably can put this as a predefined
	min.x = min.y = FLT_MAX;
	max.x = max.y = FLT_MIN;
	for (auto &p : outline) {
		if (p->x < min.x) min.x = p->x;
		if (p->y < min.y) min.y = p->y;
		if (p->x > max.x) max.x = p->x;
		if (p->y > max.y) max.y = p->y;
	}

	for (side = 0; side < 2; side++) {
		for (auto &p : m_board->Pins()) {
			// auto p = pin.get();
			int l, r;
			int jump = 1;
			Point fp;

			l = 0;
			r = 0;

			for (size_t i = 0; i < outline.size() - 1; i++) {
				Point &pa = *outline[i];
				Point &pb = *outline[i + 1];

				// jump double/dud points
				if (pa.x == pb.x && pa.y == pb.y) continue;

				// if we encounter our hull/poly start point, then we've now created the
				// closed
				// hull, jump the next segment and reset the first-point
				if ((!jump) && (fp.x == pb.x) && (fp.y == pb.y)) {
					if (i < outline.size() - 2) {
						fp   = *outline[i + 2];
						jump = 1;
						i++;
					}
				} else {
					jump = 0;
				}

				// test to see if this segment makes the scan-cut.
				if ((pa.y > pb.y && p->position.y < pa.y && p->position.y > pb.y) ||
				    (pa.y < pb.y && p->position.y > pa.y && p->position.y < pb.y)) {
					ImVec2 intersect;

					intersect.y = p->position.y;
					if (pa.x == pb.x)
						intersect.x = pa.x;
					else
						intersect.x = (pb.x - pa.x) / (pb.y - pa.y) * (p->position.y - pa.y) + pa.x;

					if (intersect.x > p->position.x)
						r++;
					else if (intersect.x < p->position.x)
						l++;
				}
			} // if we did get an intersection

			// If either side has no intersections, then it's out of bounds (likely)
			if ((l % 2 == 0) && (r % 2 == 0)) epc[side]++;
		} // pins

		if (debug) fprintf(stderr, "EPC[%d]: %d\n", side, epc[side]);

		// flip the outline
		for (auto &p : outline) p->y = max.y - p->y;

	} // side

	if ((epc[0] || epc[1]) && (epc[0] > epc[1])) {
		for (auto &p : outline) p->y = max.y - p->y;
	}

	return 0;
}

/*
 * Experimenting to see how much CPU hit rescanning and
 * drawing the flood fill is (as pin-stripe) each time
 * as opposed to precalculated line list
 *
 * We also do this slightly differently, we ask for a
 * y-pixel delta and thickness of line
 *
 */
void BoardView::OutlineGenFillDraw(ImDrawList *draw, int ydelta, double thickness = 1.0f) {

	auto io = ImGui::GetIO();
	vector<ImVec2> scanhits;
	auto &outline = m_board->OutlinePoints();
	static ImVec2 min, max; // board min/max points
	                        //	double steps = 500;
	double vdelta;
	double y, ystart, yend;

	if (!boardFill || slowCPU) return;
	if (!m_file) return;

	scanhits.reserve(20);

	draw->ChannelsSetCurrent(kChannelFill);

	// find the orthagonal bounding box
	// probably can put this as a predefined
	if (!boardMinMaxDone) {
		min.x = min.y = 100000000000;
		max.x = max.y = -100000000000;
		for (auto &p : outline) {
			if (p->x < min.x) min.x = p->x;
			if (p->y < min.y) min.y = p->y;
			if (p->x > max.x) max.x = p->x;
			if (p->y > max.y) max.y = p->y;
		}
		boardMinMaxDone = true;
	}

	// Get the viewport limits, so we don't waste time scanning what we don't need
	ImVec2 vpa = ScreenToCoord(0, 0);
	ImVec2 vpb = ScreenToCoord(io.DisplaySize);

	if (vpa.y > vpb.y) {
		ystart = vpb.y;
		yend   = vpa.y;
	} else {
		ystart = vpa.y;
		yend   = vpb.y;
	}

	if (ystart < min.y) ystart = min.y;
	if (yend > max.y) yend = max.y;

	//	vdelta = (ystart - yend) / steps;

	vdelta = ydelta / m_scale;

	/*
	 * Go through each scan line
	 */
	y = ystart;
	while (y < yend) {

		scanhits.resize(0);
		// scan outline segments to see if any intersect with our horizontal scan line

		/*
		 * While we haven't yet exhausted possible scan hits
		 */

		{

			int jump = 1;
			Point fp;

			auto &outline = m_board->OutlinePoints();

			// set our initial draw point, so we can detect when we encounter it again
			fp = *outline[0];

			for (size_t i = 0; i < outline.size() - 1; i++) {
				Point &pa = *outline[i];
				Point &pb = *outline[i + 1];

				// jump double/dud points
				if (pa.x == pb.x && pa.y == pb.y) continue;

				// if we encounter our hull/poly start point, then we've now created the
				// closed
				// hull, jump the next segment and reset the first-point
				if ((!jump) && (fp.x == pb.x) && (fp.y == pb.y)) {
					if (i < outline.size() - 2) {
						fp   = *outline[i + 2];
						jump = 1;
						i++;
					}
				} else {
					jump = 0;
				}

				// test to see if this segment makes the scan-cut.
				if ((pa.y > pb.y && y < pa.y && y > pb.y) || (pa.y < pb.y && y > pa.y && y < pb.y)) {
					ImVec2 intersect;

					intersect.y = y;
					if (pa.x == pb.x)
						intersect.x = pa.x;
					else
						intersect.x = (pb.x - pa.x) / (pb.y - pa.y) * (y - pa.y) + pa.x;
					scanhits.push_back(intersect);
				}
			} // if we did get an intersection

			sort(scanhits.begin(), scanhits.end(), [](ImVec2 const &a, ImVec2 const &b) { return a.x < b.x; });

			// now finally generate the lines.
			{
				int i = 0;
				int l = scanhits.size() - 1;
				for (i = 0; i < l; i += 2) {
					draw->AddLine(
					    CoordToScreen(scanhits[i].x, y), CoordToScreen(scanhits[i + 1].x, y), m_colors.boardFillColor, thickness);
				}
			}
		}
		y += vdelta;
	} // for each scan line
}

void BoardView::DrawDiamond(ImDrawList *draw, ImVec2 c, double r, uint32_t color) {
	ImVec2 dia[4];

	dia[0] = ImVec2(c.x, c.y - r);
	dia[1] = ImVec2(c.x + r, c.y);
	dia[2] = ImVec2(c.x, c.y + r);
	dia[3] = ImVec2(c.x - r, c.y);

	draw->AddPolyline(dia, 4, color, true, 1.0f);
}

void BoardView::DrawHex(ImDrawList *draw, ImVec2 c, double r, uint32_t color) {
	double da, db;
	ImVec2 hex[6];

	da = r * 0.5;         // cos(60')
	db = r * 0.866025404; // sin(60')

	hex[0] = ImVec2(c.x - r, c.y);
	hex[1] = ImVec2(c.x - da, c.y - db);
	hex[2] = ImVec2(c.x + da, c.y - db);
	hex[3] = ImVec2(c.x + r, c.y);
	hex[4] = ImVec2(c.x + da, c.y + db);
	hex[5] = ImVec2(c.x - da, c.y + db);

	draw->AddPolyline(hex, 6, color, true, 1.0f);
}

inline void BoardView::DrawOutline(ImDrawList *draw) {
	int jump = 1;
	Point fp;

	auto &outline = m_board->OutlinePoints();
	if (outline.empty()) {
		return;
	}

	draw->ChannelsSetCurrent(kChannelPolylines);

	// set our initial draw point, so we can detect when we encounter it again
	fp = *outline[0];

	draw->PathClear();
	for (size_t i = 0; i < outline.size() - 1; i++) {
		Point &pa = *outline[i];
		Point &pb = *outline[i + 1];

		// jump double/dud points
		if (pa == pb) continue;

		// if we encounter our hull/poly start point, then we've now created the
		// closed
		// hull, jump the next segment and reset the first-point
		if (!jump && fp == pb) {
			if (i < outline.size() - 2) {
				fp   = *outline[i + 2];
				jump = 1;
				i++;
			}
		} else {
			jump = 0;
		}

		ImVec2 spa = CoordToScreen(pa);
		ImVec2 spb = CoordToScreen(pb);

		/*
		 * If we have a pin selected, we mask off the colour to shade out
		 * things and make it easier to see associated pins/points
		 */
		if ((pinSelectMasks) && (m_pinSelected || m_pinHighlighted.size())) {
			draw->AddLine(spa, spb, (m_colors.boardOutlineColor & m_colors.selectedMaskOutline) | m_colors.orMaskOutline);
		} else {
			draw->AddLine(spa, spb, m_colors.boardOutlineColor);
		}
	} // for
}

void BoardView::DrawNetWeb(ImDrawList *draw) {
	if (!showNetWeb) return;

	/*
	 * Some nets we don't bother showing, because they're not relevant or
	 * produce too many results (such as ground!)
	 */
	if (m_pinSelected->type == Pin::kPinTypeNotConnected) return;
	if (m_pinSelected->type == Pin::kPinTypeUnkown) return;
	if (m_pinSelected->net->is_ground) return;

	for (auto &p : m_board->Pins()) {

		if (p->net == m_pinSelected->net) {
			uint32_t col = m_colors.pinNetWebColor;
			if (!BoardElementIsVisible(p->component)) {
				col = m_colors.pinNetWebOSColor;
				draw->AddCircle(CoordToScreen(p->position), p->diameter * m_scale, col, 16);
			}

			draw->AddLine(CoordToScreen(m_pinSelected->position),
			              CoordToScreen(p->position),
			              ImColor(col),
			              netWebThickness);
		}
	}

	return;
}

inline void BoardView::DrawPins(ImDrawList *draw) {

	uint32_t cmask  = 0xFFFFFFFF;
	uint32_t omask  = 0x00000000;
	float threshold = 0;
	auto io         = ImGui::GetIO();

	if (!showPins) return;

	auto apply_shade = [] (uint32_t c, uint32_t s, float i) {
		uint32_t cc = c;
		if (s) {
			cc = s;
		}
		auto f = [i, cc] (int sh) {
			uint32_t ret = (cc >> sh & 0xff) * i;
			if (ret > 255) return uint32_t(255) << sh;
			return ret << sh;
		};

		return f(0) | f(8) | f(16) | f(24);
	};
	
	/*
	 * If we have a pin selected, then it makes it
	 * easier to see where the associated pins are
	 * by masking out (alpha or channel) the other
	 * pins so they're fainter.
	 */
	if (pinSelectMasks) {
		if (m_pinSelected || m_pinHighlighted.size()) {
			cmask = m_colors.selectedMaskPins;
			omask = m_colors.orMaskPins;
		}
	}

	if (slowCPU) {
		threshold      = 2.0f;
	}
	if (pinSizeThresholdLow > threshold) threshold = pinSizeThresholdLow;

	draw->ChannelsSetCurrent(kChannelPins);

	if (m_pinSelected) DrawNetWeb(draw);

	for (auto &pin : m_board->Pins()) {
		float psz           = pin->diameter * m_scale;
		uint32_t fill_color = 0xFFFF8888; // fallback fill colour
		uint32_t text_color = m_colors.pinDefaultTextColor;
		uint32_t color      = (m_colors.pinDefaultColor & cmask) | omask;
		bool fill_pin       = false;
		bool show_text      = false;
		bool draw_ring      = true;

		// continue if pin is not visible anyway
		if (!BoardElementIsVisible(pin)) continue;

		ImVec2 pos = CoordToScreen(pin->position);
		{
			if (!IsVisibleScreen(pos.x, pos.y, psz, io)) continue;
		}

		if ((!m_pinSelected) && (psz < threshold)) continue;

		// color & text depending on app state & pin type

		{
			/*
			 * Pins resulting from a net search
			 */
			if (contains(pin, m_pinHighlighted)) {
				if (psz < fontSize / 2) psz = fontSize / 2;
				text_color = m_colors.pinSelectedTextColor;
				fill_color = m_colors.pinSelectedFillColor;
				color      = m_colors.pinSelectedColor;
				// text_color = color = m_colors.pinSameNetColor;
				fill_pin  = true;
				show_text = true;
				draw_ring = true;
				threshold = 0;
				//				draw->AddCircle(ImVec2(pos.x, pos.y), psz * pinHaloDiameter, ImColor(0xff0000ff), 32);
			}

			/*
			 * If the part is selected, as part of search or otherwise
			 */
			if (PartIsHighlighted(pin->component)) {
				color      = m_colors.pinDefaultColor;
				text_color = m_colors.pinDefaultTextColor;
				fill_pin   = false;
				draw_ring  = true;
				show_text  = true;
				threshold  = 0;
			}

			if (pin->type == Pin::kPinTypeTestPad) {
				color      = (m_colors.pinTestPadColor & cmask) | omask;
				fill_color = (m_colors.pinTestPadFillColor & cmask) | omask;
				show_text  = false;
			}

			// If the part itself is highlighted ( CVMShowPins )
			// if (p_pin->component->visualmode == p_pin->component->CVMSelected) {
			if (pin->component->visualmode == pin->component->CVMSelected) {
				color      = m_colors.pinDefaultColor;
				text_color = m_colors.pinDefaultTextColor;
				fill_pin   = false;
				draw_ring  = true;
				show_text  = true;
				threshold  = 0;
			}

			if (!pin->net || pin->type == Pin::kPinTypeNotConnected) {
				color = (m_colors.pinNotConnectedColor & cmask) | omask;
			} else {
				if (pin->net->is_ground) color = (m_colors.pinGroundColor & cmask) | omask;
			}

			// pin is on the same net as selected pin: highlight > rest
			if (m_pinSelected && pin->net == m_pinSelected->net) {
				if (psz < fontSize / 2) psz = fontSize / 2;
				color      = m_colors.pinSameNetColor;
				text_color = m_colors.pinSameNetTextColor;
				fill_color = m_colors.pinSameNetFillColor;
				draw_ring  = false;
				fill_pin   = true;
				show_text  = true; // is this something we want? Maybe an optional thing?
				threshold  = 0;
			}

			// pin selected overwrites everything
			// if (p_pin == m_pinSelected) {
			if (pin == m_pinSelected) {
				if (psz < fontSize / 2) psz = fontSize / 2;
				color      = m_colors.pinSelectedColor;
				text_color = m_colors.pinSelectedTextColor;
				fill_color = m_colors.pinSelectedFillColor;
				draw_ring  = false;
				show_text  = true;
				fill_pin   = true;
				threshold  = 0;
			}

			// Check for BGA pin '1'
			//
			if (pin->name == "A1") {
				color = fill_color = m_colors.pinA1PadColor;
				fill_pin           = m_colors.pinA1PadColor;
				draw_ring          = false;
			}

			if ((pin->number == "1")) {
				if (pin->component->pins.size() >= static_cast<unsigned int>(pinA1threshold)) { // pinA1threshold is never negative
					color = fill_color = m_colors.pinA1PadColor;
					fill_pin           = m_colors.pinA1PadColor;
					draw_ring          = false;
				}
			}

			// don't show text if it doesn't make sense
			if (pin->component->pins.size() <= 1) show_text = false;
			if (pin->type == Pin::kPinTypeTestPad) show_text = false;
		}

		color = apply_shade(color, 0, pin->component->intensity_delta_ * m_default_intensity);
		
		// Drawing
		{
			int segments;
			//			draw->ChannelsSetCurrent(kChannelImages);

			// for the round pin representations, choose how many circle segments need
			// based on the pin size
			segments = round(psz);
			if (segments > 32) segments = 32;
			if (segments < 8) segments = 8;
			float h = psz / 2 + 0.5f;

			/*
			 * if we're going to be showing the text of a pin, then we really
			 * should make sure that the drawn pin is at least as big as a single
			 * character so it doesn't look messy
			 */
			if ((show_text) && (psz < fontSize / 2)) psz = fontSize / 2;

			switch (pin->type) {
				case Pin::kPinTypeTestPad:
					if ((psz > 3) && (!slowCPU)) {
						draw->AddCircleFilled(ImVec2(pos.x, pos.y), psz, fill_color, segments);
						draw->AddCircle(ImVec2(pos.x, pos.y), psz, color, segments);
					} else if (psz > threshold) {
						draw->AddRectFilled(ImVec2(pos.x - h, pos.y - h), ImVec2(pos.x + h, pos.y + h), fill_color);
					}
					break;
				default:
					if ((psz > 3) && (psz > threshold)) {
						if (pinShapeSquare || slowCPU) {
							if (fill_pin)
								draw->AddRectFilled(ImVec2(pos.x - h, pos.y - h), ImVec2(pos.x + h, pos.y + h), fill_color);
							if (draw_ring) draw->AddRect(ImVec2(pos.x - h, pos.y - h), ImVec2(pos.x + h, pos.y + h), color);
						} else {
							if (fill_pin) draw->AddCircleFilled(ImVec2(pos.x, pos.y), psz, fill_color, segments);
							if (draw_ring) draw->AddCircle(ImVec2(pos.x, pos.y), psz, color, segments);
						}
					} else if (psz > threshold) {
						if (fill_pin) draw->AddRectFilled(ImVec2(pos.x - h, pos.y - h), ImVec2(pos.x + h, pos.y + h), fill_color);
						if (draw_ring) draw->AddRect(ImVec2(pos.x - h, pos.y - h), ImVec2(pos.x + h, pos.y + h), color);
					}
			}

			// if (p_pin == m_pinSelected) {
			//		if (pin.get() == m_pinSelected) {
			//			draw->AddCircle(ImVec2(pos.x, pos.y), psz + 1.25, m_colors.pinSelectedTextColor, segments);
			//		}

			//		if ((color == m_colors.pinSameNetColor) && (pinHalo == true)) {
			//			draw->AddCircle(ImVec2(pos.x, pos.y), psz * pinHaloDiameter, m_colors.pinHaloColor, segments,
			// pinHaloThickness);
			//		}

			if (show_text) {
				ImVec2 text_size = ImGui::CalcTextSize(pin->name.c_str());
				ImVec2 pos_adj   = ImVec2(pos.x - text_size.x * 0.5f, pos.y - text_size.y * 0.5f);

				draw->ChannelsSetCurrent(kChannelText);
				draw->AddText(pos_adj, text_color, pin->name.c_str());
				draw->ChannelsSetCurrent(kChannelPins);
			}
		}
	}
}

bool BoardView::DrawPartSymbol(ImDrawList * draw, Component * c) {
	auto rot = [](ImVec2 const & a, ImVec2 const & b, float angle) -> ImVec2 {
		ImVec2 ret = b - a;
		return { ret.x * cosf(angle) - ret.y * sinf(angle), ret.x * sinf(angle) + ret.y * cosf(angle) };
	};

	auto apply_shade = [] (uint32_t c, uint32_t s, float i) {
		uint32_t cc = c;
		if (s) {
			cc = s;
		}
		auto f = [i, cc] (int sh) {
			uint32_t ret = (cc >> sh & 0xff) * i;
			if (ret > 255) return uint32_t(255) << sh;
			return ret << sh;
		};

		return f(0) | f(8) | f(16) | f(24);
	};

	
	const float PI = M_PI;
	uint32_t color = apply_shade(0xff666666, c->shade_color_, m_default_intensity * c->intensity_delta_);
	float thick = 5;
	
	if (c->pins.size() == 2) {
		Point pp1 = c->pins[0]->position, pp2 = c->pins[1]->position;
		
		ImVec2 p1 = { pp1.x, pp1.y }, p2 = { pp2.x, pp2.y };
		
		if (c->component_type == Component::kComponentTypeCapacitor || c->name[0] == 'C' || c->name[1] == 'C') {
			const float leadlen  = 0.4f;
			const float platelen = 0.8f;

			ImVec2 mp1 = p1 - (p1 - p2) * leadlen, mp2 = p2 - (p2 - p1) * leadlen;
			
			draw->AddLine(CoordToScreen(p1), CoordToScreen(mp1), color, thick);
			draw->AddLine(CoordToScreen(p2), CoordToScreen(mp2), color, thick);
			
			ImVec2 rp1 = rot(p1, mp1, PI / 2);
			ImVec2 rp2 = rot(p2, mp2, PI / 2);
			
			draw->AddLine(CoordToScreen(mp1), CoordToScreen(mp1 + rp1 * platelen), color, thick);
			draw->AddLine(CoordToScreen(mp2), CoordToScreen(mp2 + rp2 * platelen), color, thick);
			
			ImVec2 rp11 = rot(p1, mp1, - PI / 2);
			ImVec2 rp21 = rot(p2, mp2, - PI / 2);
			draw->AddLine(CoordToScreen(mp1), CoordToScreen(mp1 + rp11 * platelen), color, thick);
			draw->AddLine(CoordToScreen(mp2), CoordToScreen(mp2 + rp21 * platelen), color, thick);

			ImVec2 dp1 = (p1 - p2) * 0.1f;
			ImVec2 dp2 = (p2 - p1) * 0.1f;

			auto dgnd = [this, draw, color, thick](ImVec2 const & p, ImVec2 const & dir, ImVec2 rp1, ImVec2 rp2) {
				for (int i = 0; i < 3; ++i) {
					rp1 = rp1 * 0.7f;
					rp2 = rp2 * 0.7f;
					draw->AddLine(CoordToScreen(p + dir * i), CoordToScreen(p + dir * i + rp1), color, thick);
					draw->AddLine(CoordToScreen(p + dir * i), CoordToScreen(p + dir * i + rp2), color, thick);
				}
			};
			
			if (c->pins[0]->net->is_ground) {
				dgnd(p1, dp1, rp1, rp2);
			}
			if (c->pins[1]->net->is_ground) {
				dgnd(p2, dp2, rp11, rp21);
			}
			return true;
		} else if (c->component_type == Component::kComponentTypeResistor || c->name[0] == 'R' || c->name[1] == 'R') {
			const int npeaks = 5;

			ImVec2 pe1 = rot(p1, p2, 60 * PI / 180) / npeaks;
			ImVec2 pe2 = rot(p1, p2, -60 * PI / 180) / npeaks;
			ImVec2 pe3 = pe1 * 2;
			ImVec2 pe4 = pe2 * 2;

			if (false && c->name == "PR8168") {
				std::cerr << p1 << " " << p2 << " " << (p1 - p2) << " " << sinf(60 * PI / 180) << " " << cosf(60 * PI / 180) << " " << pe1 << " " << pe2 << " " << pe3 << " " << pe4 << "\n";
			}
			
			ImVec2 pprev = p1;
			ImVec2 pnext = p1 + pe1;
			//float sign = 1.0f;
			for (int pi = 0; pi < npeaks + 1; ++pi) {
				draw->AddLine(CoordToScreen(pprev), CoordToScreen(pnext), color, thick);
				pprev = pnext;
				if (pi == npeaks - 1) {
					pnext = pnext + pe2;
				} else if (pi % 2 == 0) {
					pnext = pnext + pe4;
				} else {
					pnext = pnext + pe3;
				}
			}
			return true;
		}
	}
	return false;
}

inline void BoardView::DrawParts(ImDrawList *draw) {
	// float psz = (float)m_pinDiameter * 0.5f * m_scale;
	double angle;
	double distance = 0;
	struct ImVec2 pva[2000], *ppp;
	uint32_t color = m_colors.partOutlineColor;
	//	int rendered   = 0;
	char p0, p1; // first two characters of the part name, code-writing
	             // convenience more than anything else

	auto apply_shade = [] (uint32_t c, uint32_t s, float i) {
		uint32_t cc = c;
		if (s) {
			cc = s;
		}
		auto f = [i, cc] (int sh) {
			uint32_t ret = (cc >> sh & 0xff) * i;
			if (ret > 255) return uint32_t(255) << sh;
			return ret << sh;
		};

		return f(0) | f(8) | f(16) | f(24);
	};
	

	draw->ChannelsSetCurrent(kChannelPolylines);
	/*
	 * If a pin has been selected, we mask out the colour to
	 * enhance (relatively) the appearance of the pin(s)
	 */
	if (pinSelectMasks && ((m_pinSelected) || m_pinHighlighted.size())) {
		color = (m_colors.partOutlineColor & m_colors.selectedMaskParts) | m_colors.orMaskParts;
	}

	for (auto &part : m_board->Components()) {
		int pincount = 0;
		double min_x, min_y, max_x, max_y, aspect;
		outline_pt dbox[4]; // default box, if there's nothing else claiming to render the part different.

		if (part->is_dummy()) continue;

		/*
		 *
		 * When we first load the board, the outline of each part isn't (yet) rendered
		 * or determined/calculated.  So, the first time we display the board we
		 * compute the outline and store it for later.
		 *
		 * This also sets the pin diameters too.
		 *
		 */
		if (!part->outline_done) { // should only need to do this once for most parts

			ppp = &pva[0];
			if (part->pins.size() == 0) {
				if (debug) fprintf(stderr, "WARNING: Drawing empty part %s\n", part->name.c_str());
				draw->AddRect(CoordToScreen(part->p1.x + DPIF(10), part->p1.y + DPIF(10)),
				              CoordToScreen(part->p2.x - DPIF(10), part->p2.y - DPIF(10)),
				              0xff0000ff);
				draw->AddText(
				    CoordToScreen(part->p1.x + DPIF(10), part->p1.y - DPIF(50)), m_colors.partTextColor, part->name.c_str());
				continue;
			}

			for (auto &pin : part->pins) {
				pincount++;

				// scale box around pins as a fallback, else either use polygon or convex
				// hull for better shape fidelity
				if (pincount == 1) {
					min_x = pin->position.x;
					min_y = pin->position.y;
					max_x = min_x;
					max_y = min_y;
				}

				if (pincount < 2000) {
					ppp->x = pin->position.x;
					ppp->y = pin->position.y;
					ppp++;
				}

				if (pin->position.x > max_x) {
					max_x = pin->position.x;

				} else if (pin->position.x < min_x) {
					min_x = pin->position.x;
				}
				if (pin->position.y > max_y) {
					max_y = pin->position.y;

				} else if (pin->position.y < min_y) {
					min_y = pin->position.y;
				}
			}

			part->omin        = ImVec2(min_x, min_y);
			part->omax        = ImVec2(max_x, max_y);
			part->centerpoint = part->omin + (part->omax - part->omin) / 2; //ImVec2((max_x - min_x) / 2 + min_x, (max_y - min_y) / 2 + min_y);

			distance = sqrt((max_x - min_x) * (max_x - min_x) + (max_y - min_y) * (max_y - min_y));

			float pin_radius = m_pinDiameter / 2.0f;

			/*
			 *
			 * Determine the size of our part's pin radius based on the distance
			 * between the extremes of the pin coordinates.
			 *
			 * All the figures below are determined empirically rather than any
			 * specific formula.
			 *
			 */
			if ((pincount < 4) && (part->name[0] != 'U') && (part->name[0] != 'Q')) {

				if ((distance > 52) && (distance < 57)) {
					// 0603
					pin_radius = 15;
					for (auto &pin : part->pins) {
						pin->diameter = pin_radius; // * 0.05;
					}

				} else if ((distance > 247) && (distance < 253)) {
					// SMC diode?
					pin_radius = 50;
					for (auto &pin : part->pins) {
						pin->diameter = pin_radius; // * 0.05;
					}

				} else if ((distance > 195) && (distance < 199)) {
					// Inductor?
					pin_radius = 50;
					for (auto &pin : part->pins) {
						pin->diameter = pin_radius; // * 0.05;
					}

				} else if ((distance > 165) && (distance < 169)) {
					// SMB diode?
					pin_radius = 35;
					for (auto &pin : part->pins) {
						pin->diameter = pin_radius; // * 0.05;
					}

				} else if ((distance > 101) && (distance < 109)) {
					// SMA diode / tant cap
					pin_radius = 30;
					for (auto &pin : part->pins) {
						pin->diameter = pin_radius; // * 0.05;
					}

				} else if ((distance > 108) && (distance < 112)) {
					// 1206
					pin_radius = 30;
					for (auto &pin : part->pins) {
						pin->diameter = pin_radius; // * 0.05;
					}

				} else if ((distance > 64) && (distance < 68)) {
					// 0805
					pin_radius = 25;
					for (auto &pin : part->pins) {
						pin->diameter = pin_radius; // * 0.05;
					}

				} else if ((distance > 18) && (distance < 22)) {
					// 0201 cap/resistor?
					pin_radius = 5;
					for (auto &pin : part->pins) {
						pin->diameter = pin_radius; // * 0.05;
					}
				} else if ((distance > 28) && (distance < 32)) {
					// 0402 cap/resistor
					pin_radius = 10;
					for (auto &pin : part->pins) {
						pin->diameter = pin_radius; // * 0.05;
					}
				}
			}

			// TODO: pin radius is stored in Pin object
			//
			//
			//
			min_x -= pin_radius;
			max_x += pin_radius;
			min_y -= pin_radius;
			max_y += pin_radius;

			if ((max_y - min_y) < 0.01)
				aspect = 0;
			else
				aspect = (max_x - min_x) / (max_y - min_y);

			dbox[0].x = dbox[3].x = min_x;
			dbox[1].x = dbox[2].x = max_x;
			dbox[0].y = dbox[1].y = min_y;
			dbox[3].y = dbox[2].y = max_y;

			p0 = part->name[0];
			p1 = part->name[1];

			/*
			 * Draw all 2~3 pin devices as if they're not orthagonal.  It's a bit more
			 * CPU
			 * overhead but it keeps the code simpler and saves us replicating things.
			 */

			if ((pincount == 3) && (abs(aspect > 0.5)) &&
			    ((strchr("DQZ", p0) || (strchr("DQZ", p1)) || strcmp(part->name.c_str(), "LED")))) {
				outline_pt *hpt;

				memcpy(part->outline, dbox, sizeof(dbox));
				part->outline_done = true;

				hpt = part->hull = (outline_pt *)malloc(sizeof(outline_pt) * 3);
				for (auto &pin : part->pins) {
					hpt->x = pin->position.x;
					hpt->y = pin->position.y;
					hpt++;
				}
				part->hull_count = 3;

				/*
				 * handle all other devices not specifically handled above
				 */
			} else if ((pincount > 1) && (pincount < 4) && ((strchr("CRLD", p0) || (strchr("CRLD", p1))))) {
				double dx, dy;
				double tx, ty;
				double armx, army;

				dx    = part->pins[1]->position.x - part->pins[0]->position.x;
				dy    = part->pins[1]->position.y - part->pins[0]->position.y;
				angle = atan2(dy, dx);

				if (((p0 == 'L') || (p1 == 'L')) && (distance > 50)) {
					pin_radius = 15;
					for (auto &pin : part->pins) {
						pin->diameter = pin_radius; // * 0.05;
					}
					army = distance / 2;
					armx = pin_radius;
				} else if (((p0 == 'C') || (p1 == 'C')) && (distance > 90)) {
					double mpx, mpy;

					pin_radius = 15;
					for (auto &pin : part->pins) {
						pin->diameter = pin_radius; // * 0.05;
					}
					army = distance / 2 - distance / 4;
					armx = pin_radius;

					mpx = dx / 2 + part->pins[0]->position.x;
					mpy = dy / 2 + part->pins[0]->position.y;
					VHRotateV(&mpx, &mpy, dx / 2 + part->pins[0]->position.x, dy / 2 + part->pins[0]->position.y, angle);

					part->expanse        = distance;
					part->centerpoint.x  = mpx;
					part->centerpoint.y  = mpy;
					part->component_type = part->kComponentTypeCapacitor;

				} else {
					armx = army = pin_radius;
				}

				// TODO: Compact this bit of code, maybe. It works at least.
				tx = part->pins[0]->position.x - armx;
				ty = part->pins[0]->position.y - army;
				VHRotateV(&tx, &ty, part->pins[0]->position.x, part->pins[0]->position.y, angle);
				// a = CoordToScreen(tx, ty);
				part->outline[0].x = tx;
				part->outline[0].y = ty;

				tx = part->pins[0]->position.x - armx;
				ty = part->pins[0]->position.y + army;
				VHRotateV(&tx, &ty, part->pins[0]->position.x, part->pins[0]->position.y, angle);
				// b = CoordToScreen(tx, ty);
				part->outline[1].x = tx;
				part->outline[1].y = ty;

				tx = part->pins[1]->position.x + armx;
				ty = part->pins[1]->position.y + army;
				VHRotateV(&tx, &ty, part->pins[1]->position.x, part->pins[1]->position.y, angle);
				// c = CoordToScreen(tx, ty);
				part->outline[2].x = tx;
				part->outline[2].y = ty;

				tx = part->pins[1]->position.x + armx;
				ty = part->pins[1]->position.y - army;
				VHRotateV(&tx, &ty, part->pins[1]->position.x, part->pins[1]->position.y, angle);
				// d = CoordToScreen(tx, ty);
				part->outline[3].x = tx;
				part->outline[3].y = ty;

				part->outline_done = true;

				// rendered = 1;

			} else {

				/*
				 * If we have (typically) a connector with a non uniform pin distribution
				 * then we can try use the minimal bounding box algorithm
				 * to give it a more sane outline
				 */
				if ((pincount >= 4) && ((strchr("UJL", p0) || strchr("UJL", p1) || (strncmp(part->name.c_str(), "CN", 2) == 0)))) {
					ImVec2 *hull;
					int hpc;

					hull = (ImVec2 *)malloc(sizeof(ImVec2) * pincount); // massive overkill since our hull will
					                                                    // only require the perimeter points
					if (hull) {
						// Find our hull
						hpc = VHConvexHull(hull, pva, pincount); // (hpc = hull pin count)

						// If we had a valid hull, then find the MBB for it
						if (hpc > 0) {
							int i;
							ImVec2 bbox[4];
							outline_pt *hpt;
							part->hull_count = hpc;

							/*
							 * compute the convex hull and then transfer
							 * points to the part
							 */
							hpt = part->hull = (outline_pt *)malloc(sizeof(outline_pt) * (hpc + 1));
							for (i = 0; i < hpc; i++) {
								hpt->x = hull[i].x;
								hpt->y = hull[i].y;
								hpt++;
							}

							VHMBBCalculate(bbox, hull, hpc, pin_radius);
							for (i = 0; i < 4; i++) {
								part->outline[i].x = bbox[i].x;
								part->outline[i].y = bbox[i].y;
							}

							part->outline_done = true;

							/*
							 * Tighten the hull, removes any small angle segments
							 * such as a sequence of pins in a line, might be an overkill
							 */
							// hpc = TightenHull(hull, hpc, 0.1f);
						}

						free(hull);
					} else {
						fprintf(stderr, "ERROR: Cannot allocate memory for convex hull generation (%s)", strerror(errno));
						memcpy(part->outline, dbox, sizeof(dbox));
						part->outline_done = true;
						//						draw->AddRect(min, max, color);
						//						rendered = 1;
					}

				} else {
					// if it wasn't at an odd angle, or wasn't large, or wasn't a connector,
					// just an ordinary
					// type part, then this is where we'll likely end up
					memcpy(part->outline, dbox, sizeof(dbox));
					part->outline_done = true;
				}
			}

			//			if (rendered == 0) {
			//				fprintf(stderr, "Part wasn't rendered (%s)\n", part->name.c_str());
			//			}

		} // if !outline_done

		if (!BoardElementIsVisible(part)) continue;

		if (part->outline_done) {

			auto f = [this] (ImVec2 const & v) {
				return CoordToScreen(v);
			};
			ImVec2 * p = part->outline;
			ImVec2 a = f(p[0]), b = f(p[1]), c = f(p[2]), d = f(p[3]);

			/*
			 * Draw the bounding box for the part
			 */

			// if (fillParts) draw->AddQuadFilled(a, b, c, d, color & 0xffeeeeee);
			if (! DrawPartSymbol(draw, part.get())) {
				
				if (fillParts && !slowCPU) draw->AddQuadFilled(a, b, c, d, apply_shade(m_colors.partFillColor, part->shade_color_, part->intensity_delta_ * m_default_intensity));
				draw->AddQuad(a, b, c, d, apply_shade(color, 0, part->intensity_delta_ * m_default_intensity));
				if (PartIsHighlighted(part)) {
					if (fillParts && !slowCPU) draw->AddQuadFilled(a, b, c, d, m_colors.partHighlightedFillColor);
					draw->AddQuad(a, b, c, d, apply_shade(m_colors.partHighlightedColor, part->shade_color_, part->intensity_delta_ * m_default_intensity));
				}
			}

			/*
			 * Draw the convex hull of the part if it has one
			 */
			if (part->hull) {
				int i;
				draw->PathClear();
				for (i = 0; i < part->hull_count; i++) {
					ImVec2 p = CoordToScreen(part->hull[i]);
					draw->PathLineTo(p);
				}
				draw->PathStroke(m_colors.partHullColor, true, 1.0f);
			}

#if 0
			/*
			 * Draw any icon/mark featuers to illustrate the part better
			 */
			if (part->component_type == part->kComponentTypeCapacitor) {
				if (part->expanse > 90) {
					int segments = trunc(part->expanse);
					if (segments < 8) segments = 8;
					if (segments > 36) segments = 36;
					draw->AddCircle(CoordToScreen(part->centerpoint.x, part->centerpoint.y),
					                (part->expanse / 3) * m_scale,
					                m_colors.partOutlineColor & 0x8fffffff,
					                segments);
				}
			}
#endif
			
			/*
			 * Draw the text associated with the box or pins if required
			 */
			if (PartIsHighlighted(part) && !part->is_dummy() && !part->name.empty()) {
				std::string text  = part->name;
				std::string mcode = part->mfgcode;

				ImVec2 text_size    = ImGui::CalcTextSize(text.c_str());
				ImVec2 mfgcode_size = ImGui::CalcTextSize(mcode.c_str());

				if ((!showInfoPanel) && (mfgcode_size.x > text_size.x)) text_size.x = mfgcode_size.x;

				float top_y = a.y;

				if (c.y < top_y) top_y = c.y;
				ImVec2 pos = ImVec2((a.x + c.x) * 0.5f, top_y);

				pos.y -= text_size.y * 2;
				if (mcode.size()) pos.y -= text_size.y;

				pos.x -= text_size.x * 0.5f;
				draw->ChannelsSetCurrent(kChannelText);

				// This is the background of the part text.
				draw->AddRectFilled(ImVec2(pos.x - DPIF(2.0f), pos.y - DPIF(2.0f)),
				                    ImVec2(pos.x + text_size.x + DPIF(2.0f), pos.y + text_size.y + DPIF(2.0f)),
				                    m_colors.partTextBackgroundColor,
				                    0.0f);
				draw->AddText(pos, m_colors.partTextColor, text.c_str());
				if ((!showInfoPanel) && (mcode.size())) {
					//	pos.y += text_size.y;
					pos.y += text_size.y + DPIF(2.0f);
					draw->AddRectFilled(ImVec2(pos.x - DPIF(2.0f), pos.y - DPIF(2.0f)),
					                    ImVec2(pos.x + text_size.x + DPIF(2.0f), pos.y + text_size.y + DPIF(2.0f)),
					                    m_colors.annotationPopupBackgroundColor,
					                    0.0f);
					draw->AddText(ImVec2(pos.x, pos.y), m_colors.annotationPopupTextColor, mcode.c_str());
				}
				draw->ChannelsSetCurrent(kChannelPolylines);
			}
		}
	} // for each part
}

void BoardView::DrawPartTooltips(ImDrawList *draw) {
	ImVec2 spos = ImGui::GetMousePos();
	ImVec2 pos  = ScreenToCoord(spos.x, spos.y);

	//	if (m_parent_occluded) return;
	if (!m_tooltips_enabled) return;
	if (spos.x > m_board_surface.x) return;
	/*
	 * I am loathing that I have to add this, but basically check every pin on the board so we can
	 * determine if we're hovering over a testpad
	 */
	for (auto &pin : m_board->Pins()) {

		if (pin->type == Pin::kPinTypeTestPad) {
			float dx   = pin->position.x - pos.x;
			float dy   = pin->position.y - pos.y;
			float dist = dx * dx + dy * dy;
			if ((dist < (pin->diameter * pin->diameter))) {
				float pd = pin->diameter * m_scale;

				draw->AddCircle(CoordToScreen(pin->position.x, pin->position.y), pd, m_colors.pinHaloColor, 32, pinHaloThickness);
				ImGui::PushStyleColor(ImGuiCol_Text, m_colors.annotationPopupTextColor);
				ImGui::PushStyleColor(ImGuiCol_PopupBg, m_colors.annotationPopupBackgroundColor);
				ImGui::BeginTooltip();
				ImGui::Text("TP[%s]%s", pin->name.c_str(), pin->net->name.c_str());
				ImGui::EndTooltip();
				ImGui::PopStyleColor(2);
				break;
			} // if in the required diameter
		}
	}

	Component * previousHoveredPart = currentlyHoveredPart.get();
	Pin * previousHoveredPin = currentlyHoveredPin.get();
	currentlyHoveredPart = nullptr;


	if (Component * part = m_tcl->currently_hovered_part()) {
		for (auto && ci : m_board->Components()) {
			if (ci.get() == part) {
				currentlyHoveredPart = ci;
				break;
			}
		}
		if (part->outline_done) {
			/*
			 * Draw the bounding box for the part
			 */
			auto f = [this](ImVec2 const & v) {
				return CoordToScreen(v);
			};
			ImVec2 * p = part->outline;
			draw->AddQuad(f(p[0]), f(p[1]), f(p[2]), f(p[3]), m_colors.partHighlightedColor, 2);
		}
	} else if (m_tcl->grab_mouse_hover()) {
	} else {
		for (auto &part : m_board->Components()) {
			int hit = 0;
			//		auto p_part = part.get();
			
			if (!BoardElementIsVisible(part)) continue;
			
			// Work out if the point is inside the hull
			{
				int i, j, n;
				outline_pt *poly;
				
				n    = 4;
				poly = part->outline;
				
				for (i = 0, j = n - 1; i < n; j = i++) {
					if (((poly[i].y > pos.y) != (poly[j].y > pos.y)) &&
						(pos.x < (poly[j].x - poly[i].x) * (pos.y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x))
						hit ^= 1;
				}
			}
			
			// If we're inside a part
			if (hit) {
				currentlyHoveredPart = part;
				if (part->outline_done) {
					/*
					 * Draw the bounding box for the part
					 */
					auto f = [this](ImVec2 const & v) {
						return CoordToScreen(v);
					};
					ImVec2 * p = part->outline;
					draw->AddQuad(f(p[0]), f(p[1]), f(p[2]), f(p[3]), m_colors.partHighlightedColor, 2);
				}
				
				float min_dist = m_pinDiameter / 2.0f;
				min_dist *= min_dist; // all distance squared
				
				currentlyHoveredPin = nullptr;
				
				for (auto &pin : currentlyHoveredPart->pins) {
					// auto p     = pin;
					float dx   = pin->position.x - pos.x;
					float dy   = pin->position.y - pos.y;
					float dist = dx * dx + dy * dy;
					if ((dist < (pin->diameter * pin->diameter)) && (dist < min_dist)) {
						currentlyHoveredPin = pin;
						//					fprintf(stderr,"Pinhit: %s\n",pin->number.c_str());
						min_dist = dist;
					} // if in the required diameter
				}     // for each pin in the part
				
				draw->ChannelsSetCurrent(kChannelAnnotations);
				
				
				
				if (currentlyHoveredPin)
					draw->AddCircle(CoordToScreen(currentlyHoveredPin->position.x, currentlyHoveredPin->position.y),
									currentlyHoveredPin->diameter * m_scale,
									m_colors.pinHaloColor,
									32,
									pinHaloThickness);
				ImGui::PushStyleColor(ImGuiCol_Text, m_colors.annotationPopupTextColor);
				ImGui::PushStyleColor(ImGuiCol_PopupBg, m_colors.annotationPopupBackgroundColor);
				ImGui::BeginTooltip();
				if (currentlyHoveredPin) {
					ImGui::Text("%s\n[%s]%s",
								currentlyHoveredPart->name.c_str(),
								(currentlyHoveredPin ? currentlyHoveredPin->name.c_str() : " "),
								(currentlyHoveredPin ? currentlyHoveredPin->net->name.c_str() : " "));
				} else {
					ImGui::Text("%s", currentlyHoveredPart->name.c_str());
				}
				ImGui::EndTooltip();
				ImGui::PopStyleColor(2);
			}
			
		} // for each part on the board
	}
	if (currentlyHoveredPin.get() != previousHoveredPin) {
		m_tcl->notify_pin_hover(currentlyHoveredPin.get(), previousHoveredPin);
	}
	if (currentlyHoveredPart.get() != previousHoveredPart) {
		m_tcl->notify_part_hover(currentlyHoveredPart.get(), previousHoveredPart);
	}

}

inline void BoardView::DrawPinTooltips(ImDrawList *draw) {
	if (!m_tooltips_enabled) return;
	draw->ChannelsSetCurrent(kChannelAnnotations);

	if (HighlightedPinIsHovered()) {
		ImGui::PushStyleColor(ImGuiCol_Text, m_colors.annotationPopupTextColor);
		ImGui::PushStyleColor(ImGuiCol_PopupBg, m_colors.annotationPopupBackgroundColor);
		ImGui::BeginTooltip();
		ImGui::Text("%s[%s]\n%s",
		            m_pinHighlightedHovered->component->name.c_str(),
		            m_pinHighlightedHovered->name.c_str(),
		            m_pinHighlightedHovered->net->name.c_str());
		ImGui::EndTooltip();
		ImGui::PopStyleColor(2);
	}
}

inline void BoardView::DrawAnnotations(ImDrawList *draw) {

	if (!showAnnotations) return;
	if (!m_tooltips_enabled) return;

	draw->ChannelsSetCurrent(kChannelAnnotations);

	for (auto &ann : m_annotations.annotations) {
		if (ann.side == m_current_side) {
			ImVec2 a, b, s;
			if (debug) fprintf(stderr, "%d:%d:%f %f: %s\n", ann.id, ann.side, ann.x, ann.y, ann.note.c_str());
			a = s = CoordToScreen(ann.x, ann.y);
			a.x += annotationBoxOffset;
			a.y -= annotationBoxOffset;
			b = ImVec2(a.x + annotationBoxSize, a.y - annotationBoxSize);

			if ((ann.hovered == true) && (m_tooltips_enabled)) {
				char buf[60];

				snprintf(buf, sizeof(buf), "%s", ann.note.c_str());
				buf[50] = '\0';

				ImGui::PushStyleColor(ImGuiCol_Text, m_colors.annotationPopupTextColor);
				ImGui::PushStyleColor(ImGuiCol_PopupBg, m_colors.annotationPopupBackgroundColor);
				ImGui::BeginTooltip();
				ImGui::Text("%c(%0.0f,%0.0f) %s %s%c%s%c\n%s%s",
				            m_current_side ? 'B' : 'T',
				            ann.x,
				            ann.y,
				            ann.net.c_str(),
				            ann.part.c_str(),
				            ann.part.size() && ann.pin.size() ? '[' : ' ',
				            ann.pin.c_str(),
				            ann.part.size() && ann.pin.size() ? ']' : ' ',
				            buf,
				            ann.note.size() > 50 ? "..." : "");

				ImGui::EndTooltip();
				ImGui::PopStyleColor(2);
			} else {
			}
			draw->AddCircleFilled(s, DPIF(2), m_colors.annotationStalkColor, 8);
			draw->AddRectFilled(a, b, m_colors.annotationBoxColor);
			draw->AddRect(a, b, m_colors.annotationStalkColor);
			draw->AddLine(s, a, m_colors.annotationStalkColor);
		}
	}
}

bool BoardView::HighlightedPinIsHovered(void) {
	ImVec2 mp  = ImGui::GetMousePos();
	ImVec2 mpc = ScreenToCoord(mp.x, mp.y); // it's faster to compute this once than convert all pins

	m_pinHighlightedHovered = nullptr;

	/*
	 * See if any of the pins listed in the m_pinHighlighted vector are hovered over
	 */
	for (auto &p : m_pinHighlighted) {
		ImVec2 a = ImVec2(p->position.x, p->position.y);
		double r = p->diameter / 2.0f;
		if ((mpc.x > a.x - r) && (mpc.x < a.x + r) && (mpc.y > a.y - r) && (mpc.y < a.y + r)) {
			m_pinHighlightedHovered = p;
			return true;
		}
	}

	/*
	 * See if any of the pins in the same network as the SELECTED pin (single) are hovered
	 */
	for (auto &p : m_board->Pins()) {
		//	auto p   = pin.get();
		double r = p->diameter / 2.0f * m_scale;
		if (m_pinSelected && p->net == m_pinSelected->net) {
			ImVec2 a = ImVec2(p->position.x, p->position.y);
			if ((mpc.x > a.x - r) && (mpc.x < a.x + r) && (mpc.y > a.y - r) && (mpc.y < a.y + r)) {
				m_pinHighlightedHovered = p;
				return true;
			}
		}
	}

	/*
	 * See if any pins of a highlighted part are hovered
	 */
	for (auto &part : m_partHighlighted) {
		for (auto &p : part->pins) {
			double r = p->diameter / 2.0f * m_scale;
			ImVec2 a = ImVec2(p->position.x, p->position.y);
			if ((mpc.x > a.x - r) && (mpc.x < a.x + r) && (mpc.y > a.y - r) && (mpc.y < a.y + r)) {
				m_pinHighlightedHovered = p;
				return true;
			}
		}
	}

	return false;
}

int BoardView::AnnotationIsHovered(void) {
	ImVec2 mp       = ImGui::GetMousePos();
	bool is_hovered = false;
	int i           = 0;

	if (!m_tooltips_enabled) return false;
	m_annotation_last_hovered = 0;

	for (auto &ann : m_annotations.annotations) {
		ImVec2 a = CoordToScreen(ann.x, ann.y);
		if ((mp.x > a.x + annotationBoxOffset) && (mp.x < a.x + (annotationBoxOffset + annotationBoxSize)) &&
		    (mp.y < a.y - annotationBoxOffset) && (mp.y > a.y - (annotationBoxOffset + annotationBoxSize))) {
			ann.hovered               = true;
			is_hovered                = true;
			m_annotation_last_hovered = i;
		} else {
			ann.hovered = false;
		}
		i++;
	}

	if (is_hovered == false) m_annotation_clicked_id = -1;

	return is_hovered;
}

/*
 * TODO
 * EXPERIMENTAL, draw an area around selected pins
 */
void BoardView::DrawSelectedPins(ImDrawList *draw) {
	ImVec2 pl[1024];
	ImVec2 hull[1024];
	int i = 0, hpc = 0;
	if ((m_pinHighlighted.size()) < 3) return;
	for (auto &p : m_pinHighlighted) {
		pl[i] = CoordToScreen(p->position.x, p->position.y);
		i++;
	}
	hpc = VHConvexHull(hull, pl, i);
	if (hpc > 3) {
		draw->AddConvexPolyFilled(hull, hpc, 0x660000ff);
	}
}

void BoardView::DrawBoard() {
	if (!m_file || !m_board) return;

	ImDrawList *draw = ImGui::GetWindowDrawList();
	if (!m_needsRedraw) {
		memcpy((void *) draw, (void *) m_cachedDrawList, sizeof(ImDrawList));
		memcpy((void *) draw->CmdBuffer.Data, (void *) m_cachedDrawCommands.Data, m_cachedDrawCommands.Size);
		return;
	}

	// Splitting channels, drawing onto those and merging back.
	draw->ChannelsSplit(NUM_DRAW_CHANNELS);

	// We draw the Parts before the Pins so that we can ascertain the needed pin
	// size for the parts based on the part/pad geometry and spacing. -Inflex
	// OutlineGenerateFill();
	//	DrawFill(draw);
	OutlineGenFillDraw(draw, boardFillSpacing, 1);
	DrawOutline(draw);
	DrawParts(draw);
	//	DrawSelectedPins(draw);
	DrawPins(draw);
	// DrawPinTooltips(draw);
	DrawPartTooltips(draw);
	DrawAnnotations(draw);

	m_tcl->imgui_draw(draw);//ImGui::GetWindowDrawList());

	draw->ChannelsMerge();

	// Copy the new draw list and cmd buffer:
	memcpy(m_cachedDrawList, draw, sizeof(ImDrawList));
	int cmds_size = draw->CmdBuffer.size() * sizeof(ImDrawCmd);
	m_cachedDrawCommands.resize(cmds_size);
	memcpy(m_cachedDrawCommands.Data, draw->CmdBuffer.Data, cmds_size);
	m_needsRedraw = false;
}
/** end of drawing region **/

int qsort_netstrings(const void *a, const void *b) {
	const char *sa = *(const char **)a;
	const char *sb = *(const char **)b;
	return strcmp(sa, sb);
}

/**
 * CenterView
 *
 * Resets the scale and transformation back to original.
 * Does NOT change the rotation (yet?)
 *
 * PLD20160621-1715
 *
 */
void BoardView::CenterView(void) {
	// ImVec2 view = ImGui::GetIO().DisplaySize;
	ImVec2 view = m_board_surface;

	float dx = 1.1f * (m_boardWidth);
	float dy = 1.1f * (m_boardHeight);
	float sx = dx > 0 ? view.x / dx : 1.0f;
	float sy = dy > 0 ? view.y / dy : 1.0f;

	//  m_rotation = 0;
	m_scale_floor = m_scale = sx < sy ? sx : sy;
	SetTarget(m_mx, m_my);
	m_needsRedraw = true;
}

void BoardView::SetFile(obv_shared_ptr<BRDFile> file, obv_shared_ptr<BRDBoard> board) {
	//delete m_file;
	//delete m_board;

	// Check board outline (format) point count.
	//		If we don't have an outline, generate one
	//
	if (file->format.size() < 3) {
		auto pins = file->pins;
		int minx, maxx, miny, maxy;
		int margin = 200; // #define or leave this be? Rather arbritary.

		minx = miny = INT_MAX;
		maxx = maxy = INT_MIN;

		for (auto a : pins) {
			if (a.pos.x > maxx) maxx = a.pos.x;
			if (a.pos.y > maxy) maxy = a.pos.y;
			if (a.pos.x < minx) minx = a.pos.x;
			if (a.pos.y < miny) miny = a.pos.y;
		}

		maxx += margin;
		maxy += margin;
		minx -= margin;
		miny -= margin;

		file->format.push_back({minx, miny});
		file->format.push_back({maxx, miny});
		file->format.push_back({maxx, maxy});
		file->format.push_back({minx, maxy});
		file->format.push_back({minx, miny});
	}

	m_file  = file;
	if (board.get()) {
		m_board = board;
	} else {
		m_board = obv_make_shared<BRDBoard>(file.get());
	}
	searcher.setParts(m_board->Components());
	searcher.setNets(m_board->Nets());

	std::vector<std::string> netnames;
	for (auto &n : m_board->Nets()) netnames.push_back(n->name);
	std::vector<std::string> partnames;
	for (auto &p : m_board->Components()) netnames.push_back(p->name);

	scnets.setDictionary(netnames);
	scparts.setDictionary(partnames);

	m_nets = m_board->Nets();

	int min_x = INT_MAX, max_x = INT_MIN, min_y = INT_MAX, max_y = INT_MIN;
	for (auto &pa : m_file->format) {
		if (pa.x < min_x) min_x = pa.x;
		if (pa.y < min_y) min_y = pa.y;
		if (pa.x > max_x) max_x = pa.x;
		if (pa.y > max_y) max_y = pa.y;
	}

	// ImVec2 view = ImGui::GetIO().DisplaySize;
	ImVec2 view = m_board_surface;

	m_mx = (float)(min_x + max_x) / 2.0f;
	m_my = (float)(min_y + max_y) / 2.0f;

	float dx = 1.1f * (max_x - min_x);
	float dy = 1.1f * (max_y - min_y);
	float sx = dx > 0 ? view.x / dx : 1.0f;
	float sy = dy > 0 ? view.y / dy : 1.0f;

	m_scale_floor = m_scale = sx < sy ? sx : sy;
	m_boardWidth            = max_x - min_x;
	m_boardHeight           = max_y - min_y;
	SetTarget(m_mx, m_my);

	m_pinHighlighted.reserve(m_board->Components().size());
	m_partHighlighted.reserve(m_board->Components().size());
	m_pinSelected = nullptr;

	m_firstFrame  = true;
	m_needsRedraw = true;
}

ImVec2 BoardView::CoordToScreen(float x, float y, float w) {
	float side = m_current_side ? -1.0f : 1.0f;

	float tx   = side  * m_scale * ((m_y_offset ? m_boardWidth - x : x)       + w * (m_dx - m_mx));
	float ty   = -1.0f * m_scale * ((m_y_offset ? 0 * 2 * m_y_offset + y : y) + w * (m_dy - m_my));

	switch (m_rotation) {
		case 0:  return ImVec2(tx, ty);
		case 1:  return ImVec2(-ty, tx);
		case 2:  return ImVec2(-tx, -ty);
		default: return ImVec2(ty, -tx);
	}
}

ImVec2 BoardView::ScreenToCoord(float x, float y, float w) {
	float tx, ty;

	switch (m_rotation) {
		case 0:
			tx = x;
			ty = y;
			break;
		case 1:
			tx = y;
			ty = -x;
			break;
		case 2:
			tx = -x;
			ty = -y;
			break;
		default:
			tx = -y;
			ty = x;
			break;
	}
	float side     = m_current_side ? -1.0f : 1.0f;
	float invscale = 1.0f / m_scale;

	ty = ty * -1.0f * invscale + w * (m_my - m_dy);

	if (m_draw_both_sides && ty < 0) {// m_boardHeight) {
		//ty = 2 * m_boardHeight - ty;
		ty = -ty;
		tx = tx * side  * invscale + w * (m_mx - m_dx);

		//tx = m_boardWidth - tx;
	} else {
		tx = tx * side  * invscale + w * (m_mx - m_dx);
	}
	return ImVec2(tx, ty);
}

void BoardView::Rotate(int count) {
	// too lazy to do math
	while (count > 0) {
		m_rotation = (m_rotation + 1) & 3;
		float dx   = m_dx;
		float dy   = m_dy;
		if (m_current_side == 0) {
			m_dx = -dy;
			m_dy = dx;
		} else {
			m_dx = dy;
			m_dy = -dx;
		}
		--count;
		m_needsRedraw = true;
	}
	while (count < 0) {
		m_rotation = (m_rotation - 1) & 3;
		float dx   = m_dx;
		float dy   = m_dy;
		if (m_current_side == 1) {
			m_dx = -dy;
			m_dy = dx;
		} else {
			m_dx = dy;
			m_dy = -dx;
		}
		++count;
		m_needsRedraw = true;
	}
}

void BoardView::Mirror(void) {
	auto &outline = m_board->OutlinePoints();
	ImVec2 min, max;

	// find the orthagonal bounding box
	// probably can put this as a predefined
	min.x = min.y = FLT_MAX;
	max.x = max.y = FLT_MIN;
	for (auto &p : outline) {
		if (p->x < min.x) min.x = p->x;
		if (p->y < min.y) min.y = p->y;
		if (p->x > max.x) max.x = p->x;
		if (p->y > max.y) max.y = p->y;
	}

	for (auto &p : outline) {
		p->x = max.x - p->x;
	}

	for (auto &p : m_board->Pins()) {
		//	auto p        = pin.get();
		p->position.x = max.x - p->position.x;
	}

	for (auto &part : m_board->Components()) {

		part->centerpoint.x = max.x - part->centerpoint.x;

		if (part->outline_done) {
			for (int i = 0; i < 4; i++) {
				part->outline[i].x = max.x - part->outline[i].x;
			}
		}

		for (int i = 0; i < part->hull_count; i++) {
			part->hull[i].x = max.x - part->hull[i].x;
		}
	}

	for (auto &ann : m_annotations.annotations) {
		ann.x = max.x - ann.x;
	}
}

void BoardView::SetTarget(float x, float y) {
	// ImVec2 view  = ImGui::GetIO().DisplaySize;
	ImVec2 view  = m_board_surface;
	ImVec2 coord = ScreenToCoord(view.x / 2.0f, view.y / 2.0f);
	m_dx += coord.x - x;
	m_dy += coord.y - y;
}

inline bool BoardView::BoardElementIsVisible(const obv_shared_ptr<BoardElement> be) {
	if (!be) return true; // no element? => no board side info

	if (be->board_side == m_current_side) return true;

	if (be->board_side == kBoardSideBoth) return true;

	return false;
}

inline bool BoardView::IsVisibleScreen(float x, float y, float radius, const ImGuiIO &io) {
	// if (x < -radius || y < -radius || x - radius > io.DisplaySize.x || y - radius > io.DisplaySize.y) return false;
	[&io] { }();
	if (x < -radius || y < -radius || x - radius > m_board_surface.x || y - radius > m_board_surface.y) return false;
	return true;
}

bool BoardView::PartIsHighlighted(const obv_shared_ptr<Component> component) {
	bool highlighted = contains(component, m_partHighlighted);

	if (false && highlighted) {
		std::cerr << "contains " << highlighted << " " << component.get() << " " << m_partHighlighted.size() << " " << m_partHighlighted[0].get() << "\n";
		char * p = nullptr;
		*p = 0;
	}
	
	// is any pin of this part selected?
	if (m_pinSelected) highlighted |= m_pinSelected->component == component;

	return highlighted;
}

bool BoardView::AnyItemVisible(void) {
	bool any_visible = false;

	if (m_searchComponents) {
		for (auto &p : m_partHighlighted) {
			any_visible |= BoardElementIsVisible(p);
		}
	}

	if (!any_visible) {
		if (m_searchNets) {
			for (auto &p : m_pinHighlighted) {
				any_visible |= BoardElementIsVisible(p->component);
			}
		}
	}

	return any_visible;
}

void BoardView::FindNetNoClear(const char *name) {
	if (!m_file || !m_board || !(*name)) return;

	auto results = searcher.nets(name);

	for (auto &net : results) {
		for (auto &pin : net->pins) m_pinHighlighted.push_back(pin);
	}
}

void BoardView::FindNet(const char *name) {
	m_pinHighlighted.clear();
	FindNetNoClear(name);
}

void BoardView::FindComponentNoClear(const char *name) {
	if (!m_file || !m_board || !name) return;

	auto results = searcher.parts(name);

	for (auto &p : results) {
		m_partHighlighted.push_back(p);

		for (auto &pin : p->pins) {
			m_pinHighlighted.push_back(pin);
		}
	}
	m_needsRedraw = true;
}

void BoardView::FindComponent(const char *name) {
	if (!m_file || !m_board) return;

	m_pinHighlighted.clear();
	m_partHighlighted.clear();

	FindComponentNoClear(name);
}

void BoardView::SearchCompoundNoClear(const char *item) {
	if (*item == '\0') return;
	if (debug) fprintf(stderr, "Searching for '%s'\n", item);
	if (m_searchComponents) FindComponentNoClear(item);
	if (m_searchNets) FindNetNoClear(item);
	if (!m_partHighlighted.empty() && !m_pinHighlighted.empty() && !AnyItemVisible())
		FlipBoard(1); // passing 1 to override flipBoard parameter
}

void BoardView::SearchCompound(const char *item) {
	if (*item == '\0') return;
	m_pinHighlighted.clear();
	m_partHighlighted.clear();
	//	ClearAllHighlights();

	SearchCompoundNoClear(item);
}

void BoardView::SetLastFileOpenName(const std::string &name) {
	m_lastFileOpenName = name;
}

void BoardView::FlipBoard(int mode) {
	ImVec2 mpos = ImGui::GetMousePos();
	// ImVec2 view = ImGui::GetIO().DisplaySize;
	ImVec2 view = m_board_surface;
	ImVec2 bpos = ScreenToCoord(mpos.x, mpos.y);
	auto io     = ImGui::GetIO();

	if (mode == 1)
		mode = 0;
	else
		mode = flipMode;

	m_current_side ^= 1;
	m_dx = -m_dx;
	if (m_flipVertically) {
		Rotate(2);
		if (io.KeyShift ^ mode) {
			SetTarget(bpos.x, bpos.y);
			Pan(DIR_RIGHT, view.x / 2 - mpos.x);
			Pan(DIR_DOWN, view.y / 2 - mpos.y);
		}
	}
	m_needsRedraw = true;
}

BitVec::~BitVec() {
	free(m_bits);
}

void BitVec::Resize(uint32_t new_size) {
	if (new_size > m_size) {
		uint32_t bytelen     = 4 * ((m_size + 31) / 32);
		uint32_t new_bytelen = 4 * ((new_size + 31) / 32);
		uint32_t *new_bits   = (uint32_t *)malloc(new_bytelen);
		if (m_bits) {
			memcpy(new_bits, m_bits, bytelen);
			free(m_bits);
			memset((char *)new_bits + bytelen, 0, new_bytelen - bytelen);
		} else {
			memset(new_bits, 0, new_bytelen);
		}
		m_bits = new_bits;
	}
	m_size = new_size;
}

void BoardView::wakeup() {
	if (m_wakeup_pipe[0] < 0) {
		pipe(m_wakeup_pipe);
	}
	write(m_wakeup_pipe[1], "\0", 1);
}

int BoardView::m_wakeup_pipe[2] = { -1, -1 };
