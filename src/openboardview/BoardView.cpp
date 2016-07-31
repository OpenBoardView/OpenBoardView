#define _CRT_SECURE_NO_WARNINGS 1
#include "BoardView.h"
#include "history.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits.h>
#include <memory>
#include <sqlite3.h>
#include <stdio.h>
#ifndef _WIN32 // SDL not used on Windows
#include <SDL2/SDL.h>
#endif

#include "BDVFile.h"
#include "BRD2File.h"
#include "BRDBoard.h"
#include "BRDFile.h"
#include "BVRFile.h"
#include "Board.h"
#include "FZFile.h"
#include "imgui/imgui.h"

#include "NetList.h"
#include "PartList.h"
#include "vectorhulls.h"

#include "platform.h"

using namespace std;
using namespace std::placeholders;

#if _MSC_VER
#define stricmp _stricmp
#endif

#ifndef _WIN32
#define stricmp strcasecmp
#endif

BoardView::~BoardView() {
	delete m_file;
	delete m_board;
	sqlite3_close(m_sql);
	free(m_lastFileOpenName);
}

static int sqlCallback(void *NotUsed, int argc, char **argv, char **azColName) {
	int i;
	for (i = 0; i < argc; i++) {
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

int BoardView::sqlInit(void) {

	char *zErrMsg = 0;
	int rc;
	char sql_table_test[] = "SELECT name FROM sqlite_master WHERE type='table' AND name='annotations'";
	char sql_table_create[] =
	    "CREATE TABLE annotations("
	    "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
	    "VISIBLE INTEGER,"
	    "PIN TEXT,"
	    "PART TEXT,"
	    "NET TEXT,"
	    "POSX INTEGER,"
	    "POSY INTEGER,"
	    "SIDE INTEGER,"
	    "NOTE TEXT );";

	if (!m_sql) return 1;

	/* Execute SQL statement */
	rc = sqlite3_exec(m_sql, sql_table_create, sqlCallback, 0, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	} else {
		fprintf(stdout, "Table created successfully\n");
	}

	return 0;
}

uint32_t BoardView::byte4swap(uint32_t x) {
	/*
	 * used to convert RGBA -> ABGR etc
	 */
	return (((x & 0x000000ff) << 24) | ((x & 0x0000ff00) << 8) | ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24));
}

int BoardView::ConfigParse(void) {
	ImGuiStyle &style = ImGui::GetStyle();

	if (obvconfig.ParseBool("lightTheme", "false")) {
		style.Colors[ImGuiCol_Text]                 = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled]         = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
		style.Colors[ImGuiCol_PopupBg]              = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
		style.Colors[ImGuiCol_WindowBg]             = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
		style.Colors[ImGuiCol_ChildWindowBg]        = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_Border]               = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
		style.Colors[ImGuiCol_BorderShadow]         = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
		style.Colors[ImGuiCol_FrameBg]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		style.Colors[ImGuiCol_FrameBgActive]        = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		style.Colors[ImGuiCol_TitleBg]              = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
		style.Colors[ImGuiCol_TitleBgActive]        = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
		style.Colors[ImGuiCol_MenuBarBg]            = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
		style.Colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
		style.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
		style.Colors[ImGuiCol_ComboBg]              = ImVec4(0.86f, 0.86f, 0.86f, 0.99f);
		style.Colors[ImGuiCol_CheckMark]            = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_SliderGrab]           = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
		style.Colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_Button]               = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		style.Colors[ImGuiCol_ButtonHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive]         = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_Header]               = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
		style.Colors[ImGuiCol_HeaderHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		style.Colors[ImGuiCol_HeaderActive]         = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_Column]               = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
		style.Colors[ImGuiCol_ColumnHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
		style.Colors[ImGuiCol_ColumnActive]         = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip]           = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
		style.Colors[ImGuiCol_ResizeGripHovered]    = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		style.Colors[ImGuiCol_ResizeGripActive]     = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		style.Colors[ImGuiCol_CloseButton]          = ImVec4(0.59f, 0.59f, 0.59f, 0.50f);
		style.Colors[ImGuiCol_CloseButtonHovered]   = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
		style.Colors[ImGuiCol_CloseButtonActive]    = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
		style.Colors[ImGuiCol_PlotLines]            = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
		style.Colors[ImGuiCol_PlotLinesHovered]     = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram]        = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg]       = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
	}

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
	if (slowCPU == true) {
		style.AntiAliasedShapes = false;
	}

	pinSizeThresholdLow = obvconfig.ParseDouble("pinSizeThresholdLow", 0);
	pinShapeSquare      = obvconfig.ParseBool("pinShapeSquare", false);
	pinShapeCircle      = obvconfig.ParseBool("pinShapeCircle", true);

	if ((!pinShapeCircle) && (!pinShapeSquare)) {
		pinShapeSquare = true;
	}

	pinHalo   = obvconfig.ParseBool("pinHalo", true);
	showFPS   = obvconfig.ParseBool("showFPS", false);
	fillParts = obvconfig.ParseBool("fillParts", true);

	zoomFactor   = obvconfig.ParseInt("zoomFactor", 10) / 10.0f;
	zoomModifier = obvconfig.ParseInt("zoomModifier", 5);

	panFactor   = obvconfig.ParseInt("panFactor", 30);
	panModifier = obvconfig.ParseInt("panModifier", 5);

	/*
	 * Colours in ImGui can be represented as a 4-byte packed uint32_t as ABGR
	 * but most humans are more accustomed to RBGA, so for the sake of readability
	 * we use the human-readable version but swap the ordering around when
	 * it comes to assigning the actual colour to ImGui.
	 */

	/*
	 * XRayBlue theme
	 */
	m_colors.backgroundColor         = byte4swap(obvconfig.ParseHex("backgroundColor", 0xeeeeeeff));
	m_colors.partTextColor           = byte4swap(obvconfig.ParseHex("partTextColor", 0xff3030ff));
	m_colors.partTextBackgroundColor = byte4swap(obvconfig.ParseHex("partTextBackgroundColor", 0xffff00ff));
	m_colors.boardOutline            = byte4swap(obvconfig.ParseHex("boardOutline", 0x444444ff));
	m_colors.boxColor                = byte4swap(obvconfig.ParseHex("boxColor", 0x444444ff));
	m_colors.pinDefault              = byte4swap(obvconfig.ParseHex("pinDefault", 0x8888ffff));
	m_colors.pinGround               = byte4swap(obvconfig.ParseHex("pinGround", 0x2222aaff));
	m_colors.pinNotConnected         = byte4swap(obvconfig.ParseHex("pinNotConnected", 0xaaaaaaff));
	m_colors.pinTestPad              = byte4swap(obvconfig.ParseHex("pinTestPad", 0x888888ff));
	m_colors.pinSelectedText         = byte4swap(obvconfig.ParseHex("pinSelectedText", 0xff0000ff));
	m_colors.pinSelected             = byte4swap(obvconfig.ParseHex("pinSelected", 0x0000ffff));
	m_colors.pinHalo                 = byte4swap(obvconfig.ParseHex("pinHaloColor", 0x00aa00ff));
	m_colors.pinHighlighted          = byte4swap(obvconfig.ParseHex("pinHighlighted", 0x0000ffff));
	m_colors.pinHighlightSameNet     = byte4swap(obvconfig.ParseHex("pinHighlightSameNet", 0x000000ff));
	m_colors.annotationPartAlias     = byte4swap(obvconfig.ParseHex("annotationPartAlias", 0xffff00ff));
	m_colors.partHullColor           = byte4swap(obvconfig.ParseHex("partHullColor", 0x80808080));

	m_colors.selectedMaskPins    = byte4swap(obvconfig.ParseHex("selectedMaskPins", 0xffffffff));
	m_colors.selectedMaskParts   = byte4swap(obvconfig.ParseHex("selectedMaskParts", 0xffffffff));
	m_colors.selectedMaskOutline = byte4swap(obvconfig.ParseHex("selectedMaskOutline", 0xffffffff));

	m_colors.orMaskPins    = byte4swap(obvconfig.ParseHex("orMaskPins", 0xccccccff));
	m_colors.orMaskParts   = byte4swap(obvconfig.ParseHex("orMaskParts", 0x787878ff));
	m_colors.orMaskOutline = byte4swap(obvconfig.ParseHex("orMaskOutline", 0x888888ff));

	/*
	 * Original dark theme
	 */
	/*
	m_colors.backgroundColor     = byte4swap(obvconfig.ParseHex("backgroundColor", 0x000000a0));
	m_colors.partTextColor       = byte4swap(obvconfig.ParseHex("partTextColor", 0x008080ff));
	m_colors.partTextBackgroundColor = byte4swap(obvconfig.ParseHex("partTextBackgroundColor", 0xeeee00ff));
	m_colors.boardOutline        = byte4swap(obvconfig.ParseHex("boardOutline", 0xffff00ff));
	m_colors.boxColor            = byte4swap(obvconfig.ParseHex("boxColor", 0xccccccff));
	m_colors.pinDefault          = byte4swap(obvconfig.ParseHex("pinDefault", 0xff0000ff));
	m_colors.pinGround           = byte4swap(obvconfig.ParseHex("pinGround", 0xbb0000ff));
	m_colors.pinNotConnected     = byte4swap(obvconfig.ParseHex("pinNotConnected", 0x0000ffff));
	m_colors.pinTestPad          = byte4swap(obvconfig.ParseHex("pinTestPad", 0x888888ff));
	m_colors.pinSelectedText     = byte4swap(obvconfig.ParseHex("pinSelectedText", 0xeeeeeeff));
	m_colors.pinSelected         = byte4swap(obvconfig.ParseHex("pinSelected", 0xeeeeeeff));
	m_colors.pinHalo             = byte4swap(obvconfig.ParseHex("pinHaloColor", 0x00ff006f));
	m_colors.pinHighlighted      = byte4swap(obvconfig.ParseHex("pinHighlighted", 0xffffffff));
	m_colors.pinHighlightSameNet = byte4swap(obvconfig.ParseHex("pinHighlightSameNet", 0xfff888ff));
	m_colors.annotationPartAlias = byte4swap(obvconfig.ParseHex("annotationPartAlias", 0xffff00ff));
	m_colors.partHullColor       = byte4swap(obvconfig.ParseHex("partHullColor", 0x80808080));

	m_colors.selectedMaskPins    = byte4swap(obvconfig.ParseHex("selectedMaskPins", 0xffffff4f));
	m_colors.selectedMaskParts   = byte4swap(obvconfig.ParseHex("selectedMaskParts", 0xffffff8f));
	m_colors.selectedMaskOutline = byte4swap(obvconfig.ParseHex("selectedMaskOutline", 0xffffff8f));

	m_colors.orMaskPins    = byte4swap(obvconfig.ParseHex("orMaskPins", 0x00000000));
	m_colors.orMaskParts   = byte4swap(obvconfig.ParseHex("orMaskParts", 0x00000000));
	m_colors.orMaskOutline = byte4swap(obvconfig.ParseHex("orMaskOutline", 0x00000000));
	*/

	/*
	 * The asus .fz file formats require a specific key to be decoded.
	 *
	 * This key is supplied in the obv.conf file as a long single line
	 * of comma/space separated 32-bit hex values 0x1234abcd etc.
	 *
	 */
	SetFZKey(obvconfig.ParseStr("FZKey", ""));

	return 0;
}

int BoardView::LoadFile(char *filename) {
	if (filename) {
		char *ext = strrchr(filename, '.');
		if (ext) {
			for (int i = 0; ext[i]; i++) ext[i] = tolower(ext[i]); // Convert extension to lowercase
		} else {
			ext = strrchr(filename, '\0'); // No extension, point to end of filename
		}

		SetLastFileOpenName(filename);
		size_t buffer_size;
		char *buffer = file_as_buffer(&buffer_size, filename);
		if (buffer) {
			BRDFile *file = nullptr;

			if (strcmp(ext, ".fz") == 0) { // Since it is encrypted we cannot use the below logic. Trust the ext.
				file = new FZFile(buffer, buffer_size, FZKey);
			} else if (BRDFile::verifyFormat(buffer, buffer_size))
				file = new BRDFile(buffer, buffer_size);
			else if (BRD2File::verifyFormat(buffer, buffer_size))
				file = new BRD2File(buffer, buffer_size);
			else if (BDVFile::verifyFormat(buffer, buffer_size))
				file = new BDVFile(buffer, buffer_size);
			else if (BVRFile::verifyFormat(buffer, buffer_size))
				file = new BVRFile(buffer, buffer_size);

			if (file && file->valid) {
				SetFile(file);
				fhistory.Prepend_save(filename);
				history_file_has_changed = 1; // used by main to know when to update the window title

				/*
				 * Now try load the DB associated with this file
				 */
				{
					int r;
					char sqlfn[1024];

					if (*ext) *ext = '_';
					snprintf(sqlfn, 1024, "%s.sqlite3", filename);
					if (*ext) *ext = '.';
					m_sql          = nullptr;

					r = sqlite3_open(sqlfn, &m_sql);
					if (r) {
						fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(m_sql));
					} else {
						if (debug) fprintf(stderr, "Opened database successfully\n");
						sqlInit();
						AnnotationGenerateList();
					}
				}

			} else {
				m_lastFileOpenWasInvalid = true;
				delete file;
			}
			free(buffer);
		}
	} else {
		return 1;
	}

	return 0;
}

void BoardView::SetFZKey(char *keytext) {

	if (keytext) {
		int ki;
		char *p, *ep, *limit;
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

void BoardView::HelpAbout(void) {
	if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		if (m_showHelpAbout) m_showHelpAbout = false;
		ImGui::Text("OpenFlex Board View");
		ImGui::Text("https://github.com/inflex/OpenBoardView");
		if (ImGui::Button("Close") || ImGui::IsKeyPressed(SDLK_ESCAPE)) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::Indent();
		ImGui::Text("License info");
		ImGui::Unindent();
		ImGui::Separator();
		ImGui::Text("OpenBoardView is MIT Licensed");
		ImGui::Text("Copyright (c) 2016 Paul Daniels (Inflex Additions)");
		ImGui::Text("Copyright (c) 2016 Chloridite");
		ImGui::Spacing();
		ImGui::Text("ImGui is MIT Licensed");
		ImGui::Text("Copyright (c) 2014-2015 Omar Cornut and ImGui contributors");
		ImGui::Separator();
		ImGui::Text("The MIT License");
		ImGui::TextWrapped(
		    "Permission is hereby granted, free of charge, to any person "
		    "obtaining a copy "
		    "of this software and associated documentation files (the "
		    "\"Software\"), to deal "
		    "in the Software without restriction, including without limitation "
		    "the rights "
		    "to use, copy, modify, merge, publish, distribute, sublicense, "
		    "and/or sell "
		    "copies of the Software, and to permit persons to whom the Software "
		    "is "
		    "furnished to do so, subject to the following conditions: ");
		ImGui::TextWrapped(
		    "The above copyright notice and this permission "
		    "notice shall be included in all "
		    "copies or substantial portions of the Software.");
		ImGui::TextWrapped(
		    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY "
		    "OF ANY KIND, EXPRESS OR "
		    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES "
		    "OF MERCHANTABILITY, "
		    "FITNESS FOR A PARTICULAR PURPOSE AND "
		    "NONINFRINGEMENT. IN NO EVENT SHALL THE "
		    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY "
		    "CLAIM, DAMAGES OR OTHER "
		    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR "
		    "OTHERWISE, ARISING FROM, "
		    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE "
		    "OR OTHER DEALINGS IN THE "
		    "SOFTWARE.");
		ImGui::EndPopup();
	}
}

void BoardView::HelpControls(void) {
	if (ImGui::BeginPopupModal("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		if (m_showHelpControls) m_showHelpControls = false;
		ImGui::Text("KEYBOARD CONTROLS");
		ImGui::SameLine();

		if (ImGui::Button("Close") || ImGui::IsKeyPressed(SDLK_ESCAPE)) {
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

		ImGui::Text("Search for compnent");
		ImGui::Text("Search for net");
		ImGui::Text("Display component list");
		ImGui::Text("Display net list");
		ImGui::Text("Clear all highlighted items");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("Flip board");
		ImGui::Spacing();
		ImGui::Spacing();

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
		ImGui::Text("Toggle FPS display");
		ImGui::Text("Toggle Position");

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

		ImGui::Text("c");
		ImGui::Text("n");
		ImGui::Text("k");
		ImGui::Text("l");
		ImGui::Text("ESC");
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("Space bar");
		ImGui::Spacing();
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

		ImGui::Text("b");
		ImGui::Text("f");
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
		ImGui::Text("Rotate board");

		ImGui::NextColumn();
		ImGui::Text("Click (on pin)");
		ImGui::Text("Click and drag");
		ImGui::Text("Scroll");
		ImGui::Text("Middle click");
		ImGui::Text("Right click");
		ImGui::Columns(1);

		ImGui::EndPopup();
	}
}

void BoardView::AnnotationGenerateList(void) {
	sqlite3_stmt *stmt;
	char sql[]    = "SELECT id,side,posx,posy,net,part,pin,note from annotations where visible=1;";
	char *zErrMsg = 0;
	int rc;

	rc = sqlite3_prepare_v2(m_sql, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		if (debug) cerr << "SELECT failed: " << sqlite3_errmsg(m_sql) << endl;
		return; // or throw
	}

	m_annotations.clear();
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		Annotation ann;
		ann.id      = sqlite3_column_int(stmt, 0);
		ann.side    = sqlite3_column_int(stmt, 1);
		ann.x       = sqlite3_column_int(stmt, 2);
		ann.y       = sqlite3_column_int(stmt, 3);
		ann.hovered = false;

		const char *p = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
		if (!p) p     = "";
		ann.net       = p;

		p         = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
		if (!p) p = "";
		ann.part  = p;

		p         = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
		if (!p) p = "";
		ann.pin   = p;

		p         = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 7));
		if (!p) p = "";
		ann.note  = p;

		fprintf(stderr,
		        "%d(%d:%f,%f) Net:%s Part:%s Pin:%s: Note:%s\nAdded\n",
		        ann.id,
		        ann.side,
		        ann.x,
		        ann.y,
		        ann.net.c_str(),
		        ann.part.c_str(),
		        ann.pin.c_str(),
		        ann.note.c_str());
		m_annotations.push_back(ann);
	}
	if (rc != SQLITE_DONE) {
		if (debug) cerr << "SELECT failed: " << sqlite3_errmsg(m_sql) << endl;
		// if you return/throw here, don't forget the finalize
	}
	sqlite3_finalize(stmt);
}

void BoardView::AnnotationAdd(int side, double x, double y, char *net, char *part, char *pin, char *note) {
	char sql[10240];
	char *zErrMsg = 0;
	int r;

	sqlite3_snprintf(sizeof(sql),
	                 sql,
	                 "INSERT into annotations ( visible, side, posx, posy, net, part, pin, note ) \
			values ( 1, %d, %0.0f, %0.0f, '%s', '%s', '%s', '%q' );",
	                 side,
	                 x,
	                 y,
	                 net,
	                 part,
	                 pin,
	                 note);

	r = sqlite3_exec(m_sql, sql, NULL, 0, &zErrMsg);
	if (r != SQLITE_OK) {
		if (debug) fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	} else {
		if (debug) fprintf(stdout, "Records created successfully\n");
	}
}

void BoardView::AnnotationRemove(int id) {
	char sql[1024];
	char *zErrMsg = 0;
	int r;

	sqlite3_snprintf(sizeof(sql), sql, "UPDATE annotations set visible = 0 where id=%d;", id);
	r = sqlite3_exec(m_sql, sql, NULL, 0, &zErrMsg);
	if (r != SQLITE_OK) {
		if (debug) fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	} else {
		if (debug) fprintf(stdout, "Records created successfully\n");
	}
}

void BoardView::AnnotationUpdate(int id, char *note) {
	char sql[10240];
	char *zErrMsg = 0;
	int r;

	sqlite3_snprintf(sizeof(sql), sql, "UPDATE annotations set note = '%q' where id=%d;", note, id);
	r = sqlite3_exec(m_sql, sql, NULL, 0, &zErrMsg);
	if (r != SQLITE_OK) {
		if (debug) fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	} else {
		if (debug) fprintf(stdout, "Records created successfully\n");
	}
}

void BoardView::ContextMenu(void) {
	static char contextbuf[10240] = "";
	char *pin, *partn, *net;
	char empty[] = "";

	double tx, ty;
	ImGuiIO &io = ImGui::GetIO();

	ImVec2 pos = ScreenToCoord(m_showContextMenuPos.x, m_showContextMenuPos.y);
	tx         = trunc(pos.x / 10) * 10;
	ty         = trunc(pos.y / 10) * 10;

	ImGui::SetNextWindowPos(m_showContextMenuPos);

	if (ImGui::BeginPopupModal("ContextOptions", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders)) {

		if (m_showContextMenu) {
			contextbuf[0]     = 0;
			m_showContextMenu = false;
		}

		if (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
			ImGui::SetKeyboardFocusHere(-1);
		} // set keyboard focus

		ImGui::Text("POS:%0.0f,%0.0f (%c) ", tx, ty, m_current_side == 0 ? 'T' : 'B');
		ImGui::SameLine();

		{
			/*
			 * we're going to go through all the possible items we can annotate at this position and offer them
			 */

			// threshold to within a pin's diameter of the pin center
			float min_dist = m_pinDiameter * 1.0f;

			min_dist *= min_dist; // all distance squared
			Pin *selection = nullptr;
			for (auto &pin : m_board->Pins()) {
				if (ComponentIsVisible(pin->component)) {
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
			    pin->component->name.c_str(),
			    pin->number.c_str(),
			    pin->net->name.c_str(),
			    pin->net->number,
			    pin->component->mount_type_str().c_str());
			    */

			m_partHighlighted.clear();
			for (auto &part : m_board->Components()) {
				int hit     = 0;
				auto p_part = part.get();

				if (!ComponentIsVisible(p_part)) continue;

				// Work out if the point is inside the hull
				{
					int i, j, n;
					outline_pt *poly;

					n    = 4;
					poly = p_part->outline;

					for (i = 0, j = n - 1; i < n; j = i++) {
						if (((poly[i].y > pos.y) != (poly[j].y > pos.y)) &&
						    (pos.x < (poly[j].x - poly[i].x) * (pos.y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x))
							hit = !hit;
					}
				} // hull test
				if (hit) {
					ImGui::Text("Part: %s", p_part->name.c_str());
					partn = strdup(p_part->name.c_str());

					ImGui::SameLine();
				}

			} // for each part

			if (selection != nullptr) {
				// snprintf(s, sizeof(s),"Pin %s[%s]", selection->component->name.c_str(), selection->number.c_str());
				ImGui::Text("Pin: %s[%s], Net: %s",
				            selection->component->name.c_str(),
				            selection->number.c_str(),
				            selection->net->name.c_str());
			}

			{

				ImGui::NewLine();
				ImGui::Text("Annotation");
				if (m_annotation_clicked_id) {
					if (ImGui::Button("View/Edit") || m_annotationedit_retain) {
						if (!m_annotationedit_retain) {
							snprintf(contextbuf, sizeof(contextbuf), "%s", m_annotations[m_annotation_clicked_id].note.c_str());
							m_annotationedit_retain = true;
							m_annotationnew_retain  = false;
						}
						ImGui::Spacing();
						ImGui::InputTextMultiline(
						    "##annotationedit", contextbuf, sizeof(contextbuf), ImVec2(600, ImGui::GetTextLineHeight() * 16));

						if (ImGui::Button("Update") || (ImGui::IsKeyPressed(SDLK_RETURN) && io.KeyShift)) {
							m_annotationedit_retain = false;
							if (debug) fprintf(stderr, "EDITDATA:'%s'\n\n", contextbuf);
							AnnotationUpdate(m_annotations[m_annotation_clicked_id].id, contextbuf);
							if (debug) fprintf(stderr, "DB updated\n");
							AnnotationGenerateList();
							if (debug) fprintf(stderr, "Ann list updated\n");
							m_needsRedraw      = true;
							m_tooltips_enabled = true;
							ImGui::CloseCurrentPopup();
						}
						ImGui::SameLine();
						if (ImGui::Button("Cancel")) {
							ImGui::CloseCurrentPopup();
							m_annotationnew_retain = false;
							m_tooltips_enabled     = true;
						}
						ImGui::Separator();
					}
				}

				if (ImGui::Button("Add New") || m_annotationnew_retain) {
					if (m_annotationnew_retain == false) {
						contextbuf[0]          = 0;
						m_annotationnew_retain = true;
					}
					m_annotationedit_retain = false;
					ImGui::Spacing();
					ImGui::InputTextMultiline(
					    "##annotationnew", contextbuf, sizeof(contextbuf), ImVec2(600, ImGui::GetTextLineHeight() * 16));

					if (ImGui::Button("Apply") || (ImGui::IsKeyPressed(SDLK_RETURN) && io.KeyShift)) {
						m_tooltips_enabled     = true;
						m_annotationnew_retain = false;
						if (debug) fprintf(stderr, "DATA:'%s'\n\n", contextbuf);
						if (selection != nullptr) {
							pin   = strdup(selection->number.c_str());
							partn = strdup(selection->component->name.c_str());
							net   = strdup(selection->net->name.c_str());
						} else {
							pin = empty;
							net = empty;
						}

						AnnotationAdd(m_current_side, tx, ty, net, partn, pin, contextbuf);
						AnnotationGenerateList();
						m_needsRedraw = true;

						if (selection != nullptr) {
							free(pin);
							free(partn);
							free(net);
						}
						ImGui::CloseCurrentPopup();
					}
					ImGui::SameLine();
					if (ImGui::Button("Cancel")) {
						ImGui::CloseCurrentPopup();
						m_tooltips_enabled     = true;
						m_annotationnew_retain = false;
					}
					ImGui::Separator();
				}

				if (ImGui::Button("Remove")) {
					AnnotationRemove(m_annotations[m_annotation_clicked_id].id);
					AnnotationGenerateList();
					m_needsRedraw = true;
					ImGui::CloseCurrentPopup();
				}
			}

			// the position.
		}

		ImGui::SameLine();
		if (ImGui::Button("Exit") || ImGui::IsKeyPressed(SDLK_ESCAPE)) {
			m_annotationnew_retain  = false;
			m_annotationedit_retain = false;
			m_tooltips_enabled      = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
		m_tooltips_enabled = true;
	}
}

void BoardView::SearchComponent(void) {
	if (ImGui::BeginPopupModal("Search for Component",
	                           nullptr,
	                           ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
	                               ImGuiWindowFlags_ShowBorders)) {
		char cs[128];
		const char *first_button  = m_search;
		const char *first_button2 = m_search2;
		const char *first_button3 = m_search3;

		if (m_showComponentSearch) {
			m_showComponentSearch = false;
		}

		// Column 1, implied.
		//
		ImGui::Columns(3);
		ImGui::Text("Component #1");

		if (ImGui::InputText("##search", m_search, 128, ImGuiInputTextFlags_CharsNoBlank)) {
			FindComponent(m_search);
		}

		if (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
			ImGui::SetKeyboardFocusHere(-1);
		} // set keyboard focus

		int buttons_left = 10;
		for (int i = 0; buttons_left && i < m_file->num_parts; i++) {
			const BRDPart &part = m_file->parts[i];
			if (utf8casestr(part.name, m_search)) {
				if (ImGui::SmallButton(part.name)) {
					FindComponent(part.name);
					snprintf(m_search, sizeof(m_search), "%s", part.name);
					first_button = part.name;
				} // button name generation
				  //				if (buttons_left == 10) {
				  //					first_button = part.name;
				  //				}
				buttons_left--;
			} // testing for match of our component partial to the part name
		}     // for each part ( search column 1 )

		ImGui::NextColumn();
		ImGui::Text("Component #2");
		if (ImGui::InputText("##search2", m_search2, 128, ImGuiInputTextFlags_CharsNoBlank)) {
			FindComponent(m_search2);
		}
		//			const char *first_button2 = m_search2;
		int buttons_left2 = 10;
		for (int i = 0; buttons_left2 && i < m_file->num_parts; i++) {
			const BRDPart &part2 = m_file->parts[i];
			if (utf8casestr(part2.name, m_search2)) {
				snprintf(cs, sizeof(cs), "%s##2", part2.name);
				if (ImGui::SmallButton(cs)) {
					FindComponent(part2.name);
					//		ImGui::CloseCurrentPopup();
					snprintf(m_search2, sizeof(m_search2), "%s", part2.name);
					first_button2 = part2.name;
				}
				//				if (buttons_left2 == 10) {
				//					first_button2 = part2.name;
				//				}
				buttons_left2--;
			}
		}

		ImGui::NextColumn();
		ImGui::Text("Component #3");
		if (ImGui::InputText("##search3", m_search3, 128, ImGuiInputTextFlags_CharsNoBlank)) {
			FindComponent(m_search3);
		}
		//			const char *first_button3 = m_search3;
		int buttons_left3 = 10;
		for (int i = 0; buttons_left3 && i < m_file->num_parts; i++) {
			const BRDPart &part3 = m_file->parts[i];
			if (utf8casestr(part3.name, m_search3)) {
				snprintf(cs, sizeof(cs), "%s##3", part3.name);
				if (ImGui::SmallButton(cs)) {
					FindComponent(part3.name);
					snprintf(m_search3, sizeof(m_search3), "%s", part3.name);
					first_button3 = part3.name;
				}
				//				if (buttons_left3 == 10) {
				//					first_button3 = part3.name;
				//				}
				buttons_left3--;
			}
		}

		// Enter and Esc close the search:
		if (ImGui::IsKeyPressed(SDLK_RETURN)) {
			FindComponent(first_button);
			FindComponentNoClear(first_button2);
			FindComponentNoClear(first_button3);
			ImGui::CloseCurrentPopup();
		} // response to keyboard ENTER

		ImGui::Columns(1); // reset back to single column mode
		ImGui::Separator();

		if (ImGui::Button("Search")) {
			FindComponent(first_button);
			FindComponentNoClear(first_button2);
			FindComponentNoClear(first_button3);
			ImGui::CloseCurrentPopup();
		} // search button

		ImGui::SameLine();
		if (ImGui::Button("Reset")) {
			FindComponent("");
			m_search[0]  = '\0';
			m_search2[0] = '\0';
			m_search3[0] = '\0';
		} // reset button

		ImGui::SameLine();
		if (ImGui::Button("Exit") || ImGui::IsKeyPressed(SDLK_ESCAPE)) {
			FindComponent("");
			m_search[0]  = '\0';
			m_search2[0] = '\0';
			m_search3[0] = '\0';
			ImGui::CloseCurrentPopup();
		} // exit button

		ImGui::SameLine();
		ImGui::Text("ENTER: Search, ESC: Exit, TAB: next field");

		ImGui::EndPopup();
	}
}

void BoardView::SearchNet(void) {
	/*
 * Network popup window (search)
 */
	if (ImGui::BeginPopupModal("Search for Net", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders)) {
		if (m_showNetfilterSearch) {
			m_showNetfilterSearch = false;
		}
		if (ImGui::InputText("##search", m_search, 128)) {
			SetNetFilter(m_search);
		}
		const char *first_button = m_search;
		if (ImGui::IsItemHovered() ||
		    (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)))
			ImGui::SetKeyboardFocusHere(-1);

		int buttons_left = 10;
		for (auto &net : m_nets) {
			if (buttons_left > 0) {
				if (utf8casestr(net->name.c_str(), m_search)) {
					if (ImGui::Button(net->name.c_str())) {
						SetNetFilter(net->name.c_str());
						ImGui::CloseCurrentPopup();
					}
					if (buttons_left == 10) {
						first_button = net->name.c_str();
					}
					buttons_left--;
				}
			}
		}

		// Enter and Esc close the search:
		if (ImGui::IsKeyPressed(SDLK_RETURN)) {
			SetNetFilter(first_button);
			ImGui::CloseCurrentPopup();
		}
		ImGui::Separator();
		if (ImGui::Button("Clear") || ImGui::IsKeyPressed(SDLK_ESCAPE)) {
			SetNetFilter("");
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		ImGui::Text("ENTER to submit, ESC to close");
		ImGui::EndPopup();
	}
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
	char *preset_filename = NULL;
	float menu_height     = 0;
	ImGuiIO &io           = ImGui::GetIO();

	/**
	 * ** FIXME
	 * This should be handled in the keyboard section, not here
	 */
	if ((io.KeyCtrl) && ImGui::IsKeyPressed(SDLK_o)) {
		open_file = true;
		// the dialog will likely eat our WM_KEYUP message for CTRL and O:
		io.KeysDown[SDL_SCANCODE_RCTRL] = false;
		io.KeysDown[SDL_SCANCODE_LCTRL] = false;
		io.KeysDown[SDLK_o]             = false;
	}

	if ((io.KeyCtrl) && ImGui::IsKeyPressed(SDLK_q)) {
		m_wantsQuit = true;
	}

	if (ImGui::BeginMainMenuBar()) {
		menu_height = ImGui::GetWindowHeight();

		/*
		 * Create these dialogs, but they're not actually
		 * visible until they're called up by the respective
		 * flags.
		 */
		SearchNet();
		SearchComponent();
		HelpControls();
		ContextMenu();
		HelpAbout();

		// ImGui::PushStyleColor(0xeeeeeeee);

		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open", "Ctrl+O")) {
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

			if (ImGui::MenuItem("Quit")) {
				m_wantsQuit = true;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View")) {
			if (ImGui::MenuItem("Flip board", "<Space>")) {
				FlipBoard();
			}
			if (ImGui::MenuItem("Rotate CW")) {
				Rotate(1);
			}
			if (ImGui::MenuItem("Rotate CCW")) {
				Rotate(-1);
			}
			if (ImGui::MenuItem("Reset View", "5")) { // actually want this to be numpad 5
				CenterView();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Toggle FPS", "f")) {
				showFPS ^= 1;
				m_needsRedraw = true;
			}
			if (ImGui::MenuItem("Toggle Position", "p")) {
				showPosition ^= 1;
				m_needsRedraw = true;
			}
			if (ImGui::MenuItem("Toggle Pin blank", "b")) {
				pinBlank ^= 1;
				m_needsRedraw = true;
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Search")) {
			if (ImGui::MenuItem("Components", "c")) {
				m_showComponentSearch = true;
			}
			if (ImGui::MenuItem("Nets", "n")) {
				m_showNetfilterSearch = true;
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Windows")) {
			if (ImGui::MenuItem("Net List", "l")) {
				m_showNetList = m_showNetList ? false : true;
			}
			if (ImGui::MenuItem("Part List", "k")) {
				m_showPartList = m_showPartList ? false : true;
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
		ImGui::Dummy(ImVec2(60, 1));

		ImGui::SameLine();
		if (ImGui::Button(" - ")) {
			Zoom(m_lastWidth / 2, m_lastHeight / 2, -zoomFactor);
		}
		ImGui::SameLine();
		if (ImGui::Button(" + ")) {
			Zoom(m_lastWidth / 2, m_lastHeight / 2, +zoomFactor);
		}
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(20, 1));
		ImGui::SameLine();
		if (ImGui::Button("-")) {
			Zoom(m_lastWidth / 2, m_lastHeight / 2, -zoomFactor / zoomModifier);
		}
		ImGui::SameLine();
		if (ImGui::Button("+")) {
			Zoom(m_lastWidth / 2, m_lastHeight / 2, +zoomFactor / zoomModifier);
		}

		ImGui::SameLine();
		ImGui::Dummy(ImVec2(20, 1));
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

		if (m_showContextMenu && m_file) {
			ImGui::OpenPopup("ContextOptions");
		}

		if (m_showHelpAbout) {
			ImGui::OpenPopup("About");
		}
		if (m_showHelpControls) {
			ImGui::OpenPopup("Controls");
		}
		if (m_showNetfilterSearch) {
			ImGui::OpenPopup("Search for Net");
		}
		if (m_showComponentSearch && m_file) {
			ImGui::OpenPopup("Search for Component");
		}
		if (m_lastFileOpenWasInvalid) {
			ImGui::OpenPopup("Error opening file");
			m_lastFileOpenWasInvalid = false;
		}

		if (ImGui::BeginPopupModal("Error opening file")) {
			ImGui::Text("There was an error opening the file: %s", m_lastFileOpenName);
			if (strstr(m_lastFileOpenName, ".fz")) {
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
		ImGui::EndMainMenuBar();
	}

	if (open_file) {
		char *filename;

		if (preset_filename) {
			filename        = strdup(preset_filename);
			preset_filename = NULL;
		} else {
			filename = show_file_picker();

			ImGuiIO &io           = ImGui::GetIO();
			io.MouseDown[0]       = false;
			io.MouseClicked[0]    = false;
			io.MouseClickedPos[0] = ImVec2(0, 0);
		}

		if (filename) {
			LoadFile(filename);
		}
	}

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
	                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

	ImGuiWindowFlags draw_surface_flags = flags | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;

	/*
	 * Status footer
	 */
	float status_height = (10.0f + ImGui::GetFontSize());
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 3.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::SetNextWindowPos(ImVec2{0, io.DisplaySize.y - status_height});
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, status_height));
	ImGui::Begin("status", nullptr, flags | ImGuiWindowFlags_NoFocusOnAppearing);
	if (m_file && m_board && m_pinSelected) {
		auto pin = m_pinSelected;
		ImGui::Text("Part: %s   Pin: %s   Net: %s   Probe: %d   (%s.)",
		            pin->component->name.c_str(),
		            pin->number.c_str(),
		            pin->net->name.c_str(),
		            pin->net->number,
		            pin->component->mount_type_str().c_str());
	} else {
		ImVec2 spos = ImGui::GetMousePos();
		ImVec2 pos  = ScreenToCoord(spos.x, spos.y);
		if (pinBlank) {
			ImGui::Text("PIN BLANK ON: Press 'b' to turn off. ");
			ImGui::SameLine();
		}
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
		ImGui::Text(" ");
	}
	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();

	/*
	 * Drawing surface, where the actual PCB/board is plotted out
	 */
	ImGui::SetNextWindowPos(ImVec2(0, menu_height));
	if (io.DisplaySize.x != m_lastWidth || io.DisplaySize.y != m_lastHeight) {
		m_lastWidth   = io.DisplaySize.x;
		m_lastHeight  = io.DisplaySize.y;
		m_needsRedraw = true;
	}
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - (status_height + menu_height)));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

	ImGui::Begin("surface", nullptr, draw_surface_flags);
	HandleInput();
	DrawBoard();
	ImGui::End();
	ImGui::PopStyleColor();

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
#ifdef _WIN32
#define KM(x) (x)
#else
#define KM(x) (((x)&0xFF) | 0x100)
#endif

	amount = amount / m_scale;

	if (io.KeyCtrl) amount /= panModifier;

	switch (direction) {
		case DIR_UP: amount = -amount;
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
	const ImGuiIO &io = ImGui::GetIO();

	if (ImGui::IsWindowHovered()) {

		if (ImGui::IsMouseDragging()) {
			ImVec2 delta = ImGui::GetMouseDragDelta();
			if ((abs(delta.x) > 500) || (abs(delta.y) > 500)) {
				delta.x = 0;
				delta.y = 0;
			} // If the delta values are crazy just drop them (happens when panning
			// off screen). 500 arbritary chosen
			ImGui::ResetMouseDragDelta();
			ImVec2 td = ScreenToCoord(delta.x, delta.y, 0);
			m_dx += td.x;
			m_dy += td.y;
			m_draggingLastFrame = true;
			m_needsRedraw       = true;
		} else {

			// Conext menu
			if (m_file && m_board && ImGui::IsMouseClicked(1)) {
				// Build context menu here, for annotations and inspection
				//
				ImVec2 spos                                        = ImGui::GetMousePos();
				if (AnnotationIsHovered()) m_annotation_clicked_id = m_annotation_last_hovered;

				m_showContextMenu    = true;
				m_showContextMenuPos = spos;
				m_tooltips_enabled   = false;
				m_needsRedraw        = true;
				if (debug) fprintf(stderr, "context click request at (%f %f)\n", spos.x, spos.y);

				// Flip the board with the middle click
			} else if (m_file && m_board && ImGui::IsMouseReleased(2)) {
				FlipBoard();

				// Else, click to select pin
			} else if (m_file && m_board && ImGui::IsMouseReleased(0) && !m_draggingLastFrame) {
				ImVec2 spos = ImGui::GetMousePos();
				ImVec2 pos  = ScreenToCoord(spos.x, spos.y);

				// threshold to within a pin's diameter of the pin center
				float min_dist = m_pinDiameter * 1.0f;
				min_dist *= min_dist; // all distance squared
				Pin *selection = nullptr;
				for (auto &pin : m_board->Pins()) {
					if (ComponentIsVisible(pin->component)) {
						float dx   = pin->position.x - pos.x;
						float dy   = pin->position.y - pos.y;
						float dist = dx * dx + dy * dy;
						if (dist < min_dist) {
							selection = pin.get();
							min_dist  = dist;
						}
					}
				}
				m_pinSelected = selection;

				// check also for a part hit.
				m_needsRedraw = true;

				m_partHighlighted.clear();
				for (auto &part : m_board->Components()) {
					int hit     = 0;
					auto p_part = part.get();

					if (!ComponentIsVisible(p_part)) continue;

					// Work out if the point is inside the hull
					{
						int i, j, n;
						outline_pt *poly;

						n    = 4;
						poly = p_part->outline;

						for (i = 0, j = n - 1; i < n; j = i++) {
							if (((poly[i].y > pos.y) != (poly[j].y > pos.y)) &&
							    (pos.x < (poly[j].x - poly[i].x) * (pos.y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x))
								hit = !hit;
						}
					}

					if (hit) {
						m_partHighlighted.push_back(p_part);
					}
				}
			} else {
				if (!m_showContextMenu) {
					if (AnnotationIsHovered()) {
						m_needsRedraw        = true;
						AnnotationWasHovered = true;
					}
				} else {
					AnnotationWasHovered = false;
					m_needsRedraw        = true;
				}
			}

			m_draggingLastFrame = false;
		}

		// Zoom:
		float mwheel = io.MouseWheel;
		if (mwheel != 0.0f) {
			mwheel *= zoomFactor;

			Zoom(io.MousePos.x, io.MousePos.y, mwheel);
		}
	}

	// if ((!io.WantCaptureKeyboard)&&(!m_annotationedit_retain)&&(!m_annotationnew_retain)) {
	if ((!io.WantCaptureKeyboard)) {

		if (ImGui::IsKeyPressed(SDLK_n)) {
			// Search for net
			m_showNetfilterSearch = true;

		} else if (ImGui::IsKeyPressed(SDLK_c)) {
			// Search for component
			m_showComponentSearch = true;

		} else if (ImGui::IsKeyPressed(SDLK_l)) {
			// Show Net List
			m_showNetList = m_showNetList ? false : true;

		} else if (ImGui::IsKeyPressed(SDLK_k)) {
			// Show Part List
			m_showPartList = m_showPartList ? false : true;

		} else if (ImGui::IsKeyPressed(SDLK_SPACE)) {
			// Flip board:
			FlipBoard();

		} else if (ImGui::IsKeyPressed(SDLK_b)) {
			pinBlank ^= 1;
			m_needsRedraw = true;

		} else if (ImGui::IsKeyPressed(SDLK_f)) {
			if (io.KeyCtrl)
				m_showComponentSearch = true;
			else
				showFPS ^= 1;
			m_needsRedraw = true;

		} else if (ImGui::IsKeyPressed(SDLK_SLASH)) {
			m_showComponentSearch = true;
			m_needsRedraw         = true;

		} else if (ImGui::IsKeyPressed(SDLK_p)) {
			showPosition ^= 1;
			m_needsRedraw = true;

		} else if (ImGui::IsKeyPressed(SDLK_z)) {
			reloadConfig  = true;
			m_needsRedraw = true;

		} else if (ImGui::IsKeyPressed(KM(SDLK_KP_PERIOD)) || ImGui::IsKeyPressed(SDLK_r) || ImGui::IsKeyPressed(SDLK_PERIOD)) {
			// Rotate board: R and period rotate clockwise; comma rotates
			// counter-clockwise
			Rotate(1);

		} else if (ImGui::IsKeyPressed(KM(SDLK_KP_0)) || ImGui::IsKeyPressed(SDLK_COMMA)) {
			Rotate(-1);

		} else if (ImGui::IsKeyPressed(KM(SDL_SCANCODE_KP_PLUS)) || ImGui::IsKeyPressed(SDLK_EQUALS)) {
			Zoom(m_lastWidth / 2, m_lastHeight / 2, zoomFactor);

		} else if (ImGui::IsKeyPressed(KM(SDL_SCANCODE_KP_MINUS)) || ImGui::IsKeyPressed(SDLK_MINUS)) {
			Zoom(m_lastWidth / 2, m_lastHeight / 2, -zoomFactor);

		} else if (ImGui::IsKeyPressed(KM(SDL_SCANCODE_KP_2)) || ImGui::IsKeyPressed(SDLK_s)) {
			Pan(DIR_DOWN, panFactor);

		} else if (ImGui::IsKeyPressed(KM(SDL_SCANCODE_KP_8)) || ImGui::IsKeyPressed(SDLK_w)) {
			Pan(DIR_UP, panFactor);

		} else if (ImGui::IsKeyPressed(KM(SDL_SCANCODE_KP_4)) || ImGui::IsKeyPressed(SDLK_a)) {
			Pan(DIR_LEFT, panFactor);

		} else if (ImGui::IsKeyPressed(KM(SDL_SCANCODE_KP_6)) || ImGui::IsKeyPressed(SDLK_d)) {
			Pan(DIR_RIGHT, panFactor);

		} else if (ImGui::IsKeyPressed(KM(SDL_SCANCODE_KP_5)) || ImGui::IsKeyPressed(SDLK_x)) {
			// Center and reset zoom
			CenterView();

		} else if (ImGui::IsKeyPressed(SDLK_ESCAPE)) {
			m_pinSelected = nullptr;
			SetNetFilter("");
			FindComponent("");
			m_search[0]   = '\0';
			m_search2[0]  = '\0';
			m_search3[0]  = '\0';
			m_needsRedraw = true;
		}
	}
}

/* END UPDATE REGION */

/** Overlay and Windows
 *
 *
 */
void BoardView::ShowNetList(bool *p_open) {
	static NetList netList(bind(&BoardView::SetNetFilter, this, _1));
	netList.Draw("Net List", p_open, m_board);
}

void BoardView::ShowPartList(bool *p_open) {
	static PartList partList(bind(&BoardView::FindComponent, this, _1));
	partList.Draw("Part List", p_open, m_board);
}

void BoardView::RenderOverlay() {
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

void BoardView::DrawDiamond(ImDrawList *draw, ImVec2 c, double r, uint32_t color) {
	ImVec2 dia[4];

	dia[0] = ImVec2(c.x, c.y - r);
	dia[1] = ImVec2(c.x + r, c.y);
	dia[2] = ImVec2(c.x, c.y + r);
	dia[3] = ImVec2(c.x - r, c.y);

	draw->AddPolyline(dia, 4, color, true, 1.0f, true);
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

	draw->AddPolyline(hex, 6, color, true, 1.0f, true);
}

inline void BoardView::DrawOutline(ImDrawList *draw) {
	int jump = 1;
	Point fp;

	auto &outline = m_board->OutlinePoints();

	// set our initial draw point, so we can detect when we encounter it again
	fp = *outline[0];

	draw->PathClear();
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

		ImVec2 spa = CoordToScreen(pa.x, pa.y);
		ImVec2 spb = CoordToScreen(pb.x, pb.y);

		/*
		 * If we have a pin selected, we mask off the colour to shade out
		 * things and make it easier to see associated pins/points
		 */
		if (m_pinSelected) {
			draw->AddLine(spa, spb, (m_colors.boardOutline & m_colors.selectedMaskOutline) | m_colors.orMaskOutline);
		} else {
			draw->AddLine(spa, spb, m_colors.boardOutline);
		}
	} // for
}

inline void BoardView::DrawPins(ImDrawList *draw) {

	uint32_t cmask  = 0xFFFFFFFF;
	uint32_t omask  = 0x00000000;
	float threshold = 0;
	auto io         = ImGui::GetIO();

	if (pinBlank) return;

	/*
	 * If we have a pin selected, then it makes it
	 * easier to see where the associated pins are
	 * by masking out (alpha or channel) the other
	 * pins so they're fainter.
	 */
	if (m_pinSelected) {
		cmask = m_colors.selectedMaskPins;
		omask = m_colors.orMaskPins;
	}

	if (slowCPU) {
		threshold      = 2.0f;
		pinShapeSquare = true;
		pinShapeCircle = false;
	}
	if (pinSizeThresholdLow > threshold) threshold = pinSizeThresholdLow;

	for (auto &pin : m_board->Pins()) {
		auto p_pin = pin.get();
		float psz  = pin->diameter * m_scale * 20.0f;

		// continue if pin is not visible anyway
		ImVec2 pos = CoordToScreen(pin->position.x, pin->position.y);
		{
			if (!ComponentIsVisible(pin->component)) continue;

			if (!IsVisibleScreen(pos.x, pos.y, psz, io)) continue;
		}

		if ((!m_pinSelected) && (psz < threshold)) continue;

		// color & text depending on app state & pin type
		uint32_t color      = (m_colors.pinDefault & cmask) | omask;
		uint32_t text_color = color;
		bool show_text      = false;

		{
			if (contains(*pin, m_pinHighlighted)) {
				text_color = color = m_colors.pinHighlighted;
				show_text          = true;
				threshold          = 0;
			}

			if (!pin->net || pin->type == Pin::kPinTypeNotConnected) {
				color = (m_colors.pinNotConnected & cmask) | omask;
			} else {
				if (pin->net->is_ground) color = (m_colors.pinGround & cmask) | omask;
			}

			if (PartIsHighlighted(*pin->component)) {
				if (!show_text) {
					color      = m_colors.pinHighlightSameNet;
					text_color = m_colors.partTextColor;
				}
				show_text = true;
				threshold = 0;
			}

			if (pin->type == Pin::kPinTypeTestPad) {
				color     = (m_colors.pinTestPad & cmask) | omask;
				show_text = false;
			}

			// pin is on the same net as selected pin: highlight > rest
			if (!show_text && m_pinSelected && pin->net == m_pinSelected->net) {
				color     = m_colors.pinHighlightSameNet;
				threshold = 0;
			}

			// pin selected overwrites everything
			if (p_pin == m_pinSelected) {
				color      = m_colors.pinSelected;
				text_color = m_colors.pinSelectedText;
				show_text  = true;
				threshold  = 0;
			}

			// don't show text if it doesn't make sense
			if (pin->component->pins.size() <= 1 || pin->type == Pin::kPinTypeTestPad) show_text = false;
		}

		// Drawing
		{
			int segments;
			draw->ChannelsSetCurrent(kChannelImages);

			// for the round pin representations, choose how many circle segments need
			// based on the pin size
			segments                    = round(psz);
			if (segments > 32) segments = 32;
			if (segments < 8) segments  = 8;
			float h                     = psz / 2 + 0.5f;

			switch (pin->type) {
				case Pin::kPinTypeTestPad:
					if (psz > 3) {
						draw->AddCircleFilled(ImVec2(pos.x, pos.y), psz, color, segments);
					} else if (psz > threshold) {
						draw->AddRectFilled(ImVec2(pos.x - h, pos.y - h), ImVec2(pos.x + h, pos.y + h), color);
					}
					break;
				default:
					if ((psz > 3) && (psz > threshold)) {
						if (pinShapeSquare) {
							draw->AddRect(ImVec2(pos.x - h, pos.y - h), ImVec2(pos.x + h, pos.y + h), color);
						} else {
							draw->AddCircle(ImVec2(pos.x, pos.y), psz, color, segments);
						}
					} else if (psz > threshold) {
						draw->AddRect(ImVec2(pos.x - h, pos.y - h), ImVec2(pos.x + h, pos.y + h), color);
					}
			}

			if (p_pin == m_pinSelected) {
				draw->AddCircle(ImVec2(pos.x, pos.y), psz + 1.25, m_colors.pinSelectedText, segments);
			}

			if ((color == m_colors.pinHighlightSameNet) && (pinHalo == true)) {
				draw->AddCircle(ImVec2(pos.x, pos.y), psz + 1.25, m_colors.pinHalo, segments);
			}

			if (show_text) {
				const char *pin_number = pin->number.c_str();

				ImVec2 text_size = ImGui::CalcTextSize(pin_number);
				ImVec2 pos_adj   = ImVec2(pos.x - text_size.x * 0.5f, pos.y - text_size.y * 0.5f);
				draw->ChannelsSetCurrent(kChannelPolylines);
				draw->AddRectFilled(ImVec2(pos_adj.x - 2.0f, pos_adj.y - 1.0f),
				                    ImVec2(pos_adj.x + text_size.x + 2.0f, pos_adj.y + text_size.y + 1.0f),
				                    m_colors.backgroundColor,
				                    3.0f);
				draw->ChannelsSetCurrent(kChannelText);
				draw->AddText(pos_adj, text_color, pin_number);
			}
		}
	}
}

inline void BoardView::DrawParts(ImDrawList *draw) {
	// float psz = (float)m_pinDiameter * 0.5f * m_scale;
	double angle;
	double distance = 0;
	struct ImVec2 pva[1000], *ppp;
	uint32_t color = m_colors.boxColor;
	//	int rendered   = 0;
	char p0, p1; // first two characters of the part name, code-writing
	             // convenience more than anything else

	/*
	 * If a pin has been selected, we mask out the colour to
	 * enhance (relatively) the appearance of the pin(s)
	 */
	if (m_pinSelected) color = (m_colors.boxColor & m_colors.selectedMaskParts) | m_colors.orMaskParts;

	for (auto &part : m_board->Components()) {
		int pincount = 0;
		double min_x, min_y, max_x, max_y, aspect;
		outline_pt dbox[4]; // default box, if there's nothing else claiming to render the part different.
		auto p_part = part.get();

		if (!ComponentIsVisible(p_part)) continue;

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
				draw->AddRect(
				    CoordToScreen(part->p1.x + 10, part->p1.y + 10), CoordToScreen(part->p2.x - 10, part->p2.y - 10), 0xff0000ff);
				draw->AddText(CoordToScreen(part->p1.x + 10, part->p1.y - 50), m_colors.partTextColor, part->name.c_str());
				//				part->component_type = part->kComponentTypeDummy;
				//				part->outline_done = true;
				continue;
			}

			for (auto pin : part->pins) {
				pincount++;

				// scale box around pins as a fallback, else either use polygon or convex
				// hull for better shape fidelity
				if (pincount == 1) {
					min_x = pin->position.x;
					min_y = pin->position.y;
					max_x = min_x;
					max_y = min_y;
				}

				if (pincount < 1000) {
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
					for (auto pin : part->pins) {
						pin->diameter = pin_radius * 0.05;
					}

				} else if ((distance > 247) && (distance < 253)) {
					// SMC diode?
					pin_radius = 50;
					for (auto pin : part->pins) {
						pin->diameter = pin_radius * 0.05;
					}

				} else if ((distance > 195) && (distance < 199)) {
					// Inductor?
					pin_radius = 50;
					for (auto pin : part->pins) {
						pin->diameter = pin_radius * 0.05;
					}

				} else if ((distance > 165) && (distance < 169)) {
					// SMB diode?
					pin_radius = 35;
					for (auto pin : part->pins) {
						pin->diameter = pin_radius * 0.05;
					}

				} else if ((distance > 101) && (distance < 109)) {
					// SMA diode / tant cap
					pin_radius = 30;
					for (auto pin : part->pins) {
						pin->diameter = pin_radius * 0.05;
					}

				} else if ((distance > 108) && (distance < 112)) {
					// 1206
					pin_radius = 30;
					for (auto pin : part->pins) {
						pin->diameter = pin_radius * 0.05;
					}

				} else if ((distance > 64) && (distance < 68)) {
					// 0805
					pin_radius = 25;
					for (auto pin : part->pins) {
						pin->diameter = pin_radius * 0.05;
					}

				} else if ((distance > 18) && (distance < 22)) {
					// 0201 cap/resistor?
					pin_radius = 5;
					for (auto pin : part->pins) {
						pin->diameter = pin_radius * 0.05;
					}
				} else if ((distance > 28) && (distance < 32)) {
					// 0402 cap/resistor
					pin_radius = 10;
					for (auto pin : part->pins) {
						pin->diameter = pin_radius * 0.05;
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
			//		bool bb_y_resized = false; // not sure why this is here PLD20160727

			//			ImVec2 min = CoordToScreen(min_x, min_y);
			//			ImVec2 max = CoordToScreen(max_x, max_y);

			p0 = p_part->name[0];
			p1 = p_part->name[1];

			/*
			 * Draw all 2~3 pin devices as if they're not orthagonal.  It's a bit more
			 * CPU
			 * overhead but it keeps the code simpler and saves us replicating things.
			 */

			if ((pincount == 3) && (abs(aspect > 0.5)) && ((strchr("DQZ", p0) || (strchr("DQZ", p1))))) {
				outline_pt *hpt;

				memcpy(part->outline, dbox, sizeof(dbox));
				part->outline_done = true;

				hpt = part->hull = (outline_pt *)malloc(sizeof(outline_pt) * 3);
				for (auto pin : part->pins) {
					hpt->x = pin->position.x;
					hpt->y = pin->position.y;
					hpt++;
				}
				part->hull_count = 3;

				/*
				 * handle all other devices not specifically handled above
				 */
			} else if ((pincount > 1) && (pincount < 4) && ((strchr("CRLD", p0) || (strchr("CRLD", p1))))) {
				//				ImVec2 a, b, c, d;
				double dx, dy;
				double tx, ty;
				double armx, army;

				dx    = part->pins[1]->position.x - part->pins[0]->position.x;
				dy    = part->pins[1]->position.y - part->pins[0]->position.y;
				angle = atan2(dy, dx);

				if (((p0 == 'L') || (p1 == 'L')) && (distance > 50)) {
					pin_radius = 15;
					for (auto pin : part->pins) {
						pin->diameter = pin_radius * 0.05;
					}
					army = distance / 2;
					armx = pin_radius;
				} else if (((p0 == 'C') || (p1 == 'C')) && (distance > 90)) {
					double mpx, mpy;

					pin_radius = 15;
					for (auto pin : part->pins) {
						pin->diameter = pin_radius * 0.05;
					}
					army = distance / 2 - distance / 4;
					armx = pin_radius;

					mpx = dx / 2 + part->pins[0]->position.x;
					mpy = dy / 2 + part->pins[0]->position.y;
					VHRotateV(&mpx, &mpy, dx / 2 + part->pins[0]->position.x, dy / 2 + part->pins[0]->position.y, angle);

					p_part->expanse        = distance;
					p_part->centerpoint.x  = mpx;
					p_part->centerpoint.y  = mpy;
					p_part->component_type = p_part->kComponentTypeCapacitor;

					/*
					 * Need to work out how to icon/mark the parts in the Component
					 * class so we don't have to recompute this each time.
					 */
					/*
				// for the round pin representations, choose how many circle segments
				// need based on the pin size
				segments = trunc(distance);
				if (segments < 8) segments = 8;
				if (segments > 36) segments = 36;
					draw->AddCircle(mp, (distance / 3) * m_scale, m_colors.boxColor & 0x8fffffff, segments);
					*/

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
				if ((pincount >= 4) &&
				    ((p0 == 'J') || (strncmp(part->name.c_str(), "CN", 2) == 0) || ((p0 == 'L') || (p1 == 'L')))) {
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
							//							rendered = 1;
						}
						// 	else {
						//	draw->AddRect(min, max, 0xff0000ff);
						//	rendered = 1;
						//						}

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
					//					draw->AddRect(min, max, color);
					//					if (fillParts) draw->AddRectFilled(min, max, color & 0x0fffffff);
					//					rendered = 1;
				}
			}

			//			if (rendered == 0) {
			//				fprintf(stderr, "Part wasn't rendered (%s)\n", part->name.c_str());
			//			}

			// if (m_annotationsVisible && part->annotation) {
			//    char *annotation = component.annotation;
			//    ImVec2 text_size = ImGui::CalcTextSize(annotation);

			//    float mid_y = (min.y + max.y) * 0.5f - text_size.y * 0.5f;
			//    ImVec2 pos = ImVec2((min.x + max.x) * 0.5f, mid_y);
			//    pos.x -= text_size.x * 0.5f;

			//    draw->ChannelsSetCurrent(kChannelAnnotations);
			//    draw->AddText(pos, m_colors.annotationPartAlias, annotation);
			//}
		} // if outline_done

		if (part->outline_done) {

			/*
			 * Draw the bounding box for the part
			 */
			ImVec2 a, b, c, d;

			a = ImVec2(CoordToScreen(part->outline[0].x, part->outline[0].y));
			b = ImVec2(CoordToScreen(part->outline[1].x, part->outline[1].y));
			c = ImVec2(CoordToScreen(part->outline[2].x, part->outline[2].y));
			d = ImVec2(CoordToScreen(part->outline[3].x, part->outline[3].y));

			if (fillParts) draw->AddQuadFilled(a, b, c, d, color & 0x0fFFFFFF);
			draw->AddQuad(a, b, c, d, color);

			/*
			 * Draw the convex hull of the part if it has one
			 */
			if (part->hull) {
				int i;
				draw->PathClear();
				for (i = 0; i < part->hull_count; i++) {
					ImVec2 p = CoordToScreen(part->hull[i].x, part->hull[i].y);
					draw->PathLineTo(p);
				}
				draw->PathStroke(m_colors.partHullColor, true, 1.0f);
			}

			/*
			 * Draw any icon/mark featuers to illustrate the part better
			 */
			if (part->component_type == part->kComponentTypeCapacitor) {
				if (part->expanse > 90) {
					int segments                = trunc(part->expanse);
					if (segments < 8) segments  = 8;
					if (segments > 36) segments = 36;
					draw->AddCircle(CoordToScreen(part->centerpoint.x, part->centerpoint.y),
					                (part->expanse / 3) * m_scale,
					                m_colors.boxColor & 0x8fffffff,
					                segments);
				}
			}

			/*
			 * Draw the text associated with the box or pins if required
			 */
			if (PartIsHighlighted(*part) && !part->is_dummy() && !part->name.empty()) {
				ImVec2 text_size = ImGui::CalcTextSize(part->name.c_str());
				float top_y      = a.y;

				if (c.y < top_y) top_y = c.y;
				ImVec2 pos             = ImVec2((a.x + c.x) * 0.5f, top_y);

				// I'm really not sure why this bb_y_resize code is here, I'm sure it had
				// a purpose I'm just not sure what :(
				//
				//			if (bb_y_resized) {
				//				pos.y -= text_size.y + 2.0f * psz;
				//			} else {
				//				pos.y -= text_size.y *2;
				//			}
				//
				pos.y -= text_size.y * 2;
				pos.x -= text_size.x * 0.5f;
				draw->ChannelsSetCurrent(kChannelPolylines);
				// draw->AddRectFilled(ImVec2(pos.x - 2.0f, pos.y - 1.0f), ImVec2(pos.x + text_size.x + 2.0f, pos.y +
				// text_size.y +
				// 1.0f), m_colors.backgroundColor, 3.0f);
				// This is the background of the part text.
				draw->AddRectFilled(ImVec2(pos.x - 2.0f, pos.y - 2.0f),
				                    ImVec2(pos.x + text_size.x + 2.0f, pos.y + text_size.y + 2.0f),
				                    m_colors.partTextBackgroundColor,
				                    0.0f);
				draw->ChannelsSetCurrent(kChannelText);
				draw->AddText(pos, m_colors.partTextColor, part->name.c_str());
			}
		}
	} // for each part
}

inline void BoardView::DrawAnnotations(ImDrawList *draw) {
	draw->ChannelsSetCurrent(kChannelAnnotations);
	for (auto &ann : m_annotations) {
		if (ann.side == m_current_side) {
			ImVec2 a, b, s;
			if (debug) fprintf(stderr, "%d:%d:%f %f: %s\n", ann.id, ann.side, ann.x, ann.y, ann.note.c_str());
			a = s = CoordToScreen(ann.x, ann.y);
			a.x += 10;
			a.y -= 10;
			b = ImVec2(a.x + 10, a.y - 10);

			if ((ann.hovered == true) && (m_tooltips_enabled)) {
				char buf[10240];
				snprintf(buf, sizeof(buf), "%s", ann.note.c_str());
				ImGui::BeginTooltip();
				ImGui::Text(buf);
				ImGui::EndTooltip();
			} else {
			}
			draw->AddRectFilled(a, b, 0x880000ff);
			draw->AddRect(a, b, 0xff000000);
			draw->AddLine(s, a, 0xff000000);
		}
	}
}

int BoardView::AnnotationIsHovered(void) {
	//	const ImGuiIO &io = ImGui::GetIO();
	ImVec2 mp       = ImGui::GetMousePos();
	bool is_hovered = false;
	int i           = 0;

	m_annotation_last_hovered = 0;

	for (auto &ann : m_annotations) {
		ImVec2 a = CoordToScreen(ann.x, ann.y);
		if ((mp.x > a.x + 10) && (mp.x < a.x + 20) && (mp.y > a.y - 20) && (mp.y < a.y - 10)) {
			if (debug) fprintf(stderr, "Hovering in annotation:%d\n", i);
			ann.hovered               = true;
			is_hovered                = true;
			m_annotation_last_hovered = i;
		} else {
			ann.hovered = false;
		}
		i++;
	}

	if (is_hovered == false) m_annotation_clicked_id = 0;

	return is_hovered;
}

void BoardView::DrawBoard() {
	if (!m_file || !m_board) return;

	ImDrawList *draw = ImGui::GetWindowDrawList();
	if (!m_needsRedraw) {
		memcpy(draw, m_cachedDrawList, sizeof(ImDrawList));
		memcpy(draw->CmdBuffer.Data, m_cachedDrawCommands.Data, m_cachedDrawCommands.Size);
		return;
	}

	// Splitting channels, drawing onto those and merging back.
	draw->ChannelsSplit(NUM_DRAW_CHANNELS);
	draw->ChannelsSetCurrent(kChannelPolylines);

	// We draw the Parts before the Pins so that we can ascertain the needed pin
	// size for the parts based on the part/pad geometry and spacing. -Inflex
	DrawOutline(draw);
	DrawParts(draw);
	DrawPins(draw);
	DrawAnnotations(draw);

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
	ImVec2 view = ImGui::GetIO().DisplaySize;

	float dx = 1.1f * (m_boardWidth);
	float dy = 1.1f * (m_boardHeight);
	float sx = dx > 0 ? view.x / dx : 1.0f;
	float sy = dy > 0 ? view.y / dy : 1.0f;

	//  m_rotation = 0;
	m_scale = sx < sy ? sx : sy;
	SetTarget(m_mx, m_my);
	m_needsRedraw = true;
}

void BoardView::SetFile(BRDFile *file) {
	delete m_file;
	delete m_board;

	m_file  = file;
	m_board = new BRDBoard(file);

	m_nets = m_board->Nets();

	int min_x = INT_MAX, max_x = INT_MIN, min_y = INT_MAX, max_y = INT_MIN;
	for (int i = 0; i < m_file->num_format; i++) {
		BRDPoint &pa            = m_file->format[i];
		if (pa.x < min_x) min_x = pa.x;
		if (pa.y < min_y) min_y = pa.y;
		if (pa.x > max_x) max_x = pa.x;
		if (pa.y > max_y) max_y = pa.y;
	}

	m_mx = (float)(min_x + max_x) / 2.0f;
	m_my = (float)(min_y + max_y) / 2.0f;

	ImVec2 view = ImGui::GetIO().DisplaySize;
	float dx    = 1.1f * (max_x - min_x);
	float dy    = 1.1f * (max_y - min_y);
	float sx    = dx > 0 ? view.x / dx : 1.0f;
	float sy    = dy > 0 ? view.y / dy : 1.0f;

	m_scale       = sx < sy ? sx : sy;
	m_boardWidth  = max_x - min_x;
	m_boardHeight = max_y - min_y;
	SetTarget(m_mx, m_my);

	m_pinHighlighted.reserve(m_board->Components().size());
	m_partHighlighted.reserve(m_board->Components().size());
	m_pinSelected = nullptr;

	m_firstFrame  = true;
	m_needsRedraw = true;
}

ImVec2 BoardView::CoordToScreen(float x, float y, float w) {
	float side = m_current_side ? -1.0f : 1.0f;
	float tx   = side * m_scale * (x + w * (m_dx - m_mx));
	float ty   = -1.0f * m_scale * (y + w * (m_dy - m_my));
	switch (m_rotation) {
		case 0: return ImVec2(tx, ty);
		case 1: return ImVec2(-ty, tx);
		case 2: return ImVec2(-tx, -ty);
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

	tx = tx * side * invscale + w * (m_mx - m_dx);
	ty = ty * -1.0f * invscale + w * (m_my - m_dy);

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

void BoardView::SetTarget(float x, float y) {
	ImVec2 view  = ImGui::GetIO().DisplaySize;
	ImVec2 coord = ScreenToCoord(view.x / 2.0f, view.y / 2.0f);
	m_dx += coord.x - x;
	m_dy += coord.y - y;
}

inline bool BoardView::ComponentIsVisible(const Component *part) {
	if (!part) return true; // no component? => no board side info

	if (part->board_side == kBoardSideBoth) return true;

	if (part->board_side == m_current_side) return true;

	return false;
}

inline bool BoardView::IsVisibleScreen(float x, float y, float radius, const ImGuiIO &io) {
	if (x < -radius || y < -radius || x - radius > io.DisplaySize.x || y - radius > io.DisplaySize.y) return false;
	return true;
}

bool BoardView::PartIsHighlighted(const Component &component) {
	bool highlighted = contains(component, m_partHighlighted);

	// is any pin of this part selected?
	if (m_pinSelected) highlighted |= m_pinSelected->component == &component;

	return highlighted;
}

void BoardView::SetNetFilter(const char *net) {
	strcpy(m_netFilter, net);
	if (!m_file || !m_board) return;

	m_pinHighlighted.clear();
	m_partHighlighted.clear();

	string net_name = string(net);

	if (!net_name.empty()) {
		bool any_visible = false;

		for (auto &net : m_board->Nets()) {
			if (is_prefix(net_name, net->name)) {
				for (auto pin : net->pins) {
					any_visible |= ComponentIsVisible(pin->component);
					m_pinHighlighted.push_back(pin);
					// highlighting all components that belong to this net
					if (!contains(*pin->component, m_partHighlighted)) {
						m_partHighlighted.push_back(pin->component);
					}
				}
			}
		}

		if (m_pinHighlighted.size() > 0) {
			if (!any_visible) FlipBoard();
			m_pinSelected = nullptr;
		}
	}

	m_needsRedraw = true;
}

void BoardView::FindComponentNoClear(const char *name) {
	if (!m_file || !m_board) return;

	string comp_name = string(name);

	if (!comp_name.empty()) {
		Component *part_found = nullptr;
		bool any_visible      = false;

		for (auto &component : m_board->Components()) {
			if (is_prefix(comp_name, component->name)) {
				auto p = component.get();
				m_partHighlighted.push_back(p);
				any_visible |= ComponentIsVisible(p);
				part_found = p;
			}
		}

		if (part_found) {
			if (!any_visible) FlipBoard();
			m_pinSelected = nullptr;

			for (auto &pin : part_found->pins) {
				m_pinHighlighted.push_back(pin);
			}
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

void BoardView::SetLastFileOpenName(char *name) {
	free(m_lastFileOpenName);
	m_lastFileOpenName = name;
}

void BoardView::FlipBoard() {
	m_current_side ^= 1;
	m_dx = -m_dx;
	if (m_flipVertically) {
		Rotate(2);
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
