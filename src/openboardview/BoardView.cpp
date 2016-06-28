#define _CRT_SECURE_NO_WARNINGS 1
#include "BoardView.h"

#include <SDL.h>
#include <algorithm>
#include <iostream>
#include <math.h>
#include <memory>
#include <stdio.h>

#include "BDVFile.h"
#include "BRDBoard.h"
#include "BRDFile.h"
#include "imgui/imgui.h"

#include "NetList.h"
#include "PartList.h"

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
	free(m_lastFileOpenName);
}

int BoardView::History_set_filename(char *f) {
	snprintf(history.fname, HISTORY_FNAME_LEN_MAX - 1, "%s", f);
	return 0;
}

int BoardView::History_load(void) {
	if (history.fname) {
		FILE *f;

		f             = fopen(history.fname, "r");
		history.count = 0;
		if (!f) return 0;

		while (history.count < HISTORY_COUNT_MAX) {
			char *r;

			r = fgets(history.history[history.count], HISTORY_FNAME_LEN_MAX, f);
			if (r) {
				history.count++;

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

	return history.count;
}

int BoardView::History_prepend_save(char *newfile) {
	if (history.fname) {
		FILE *f;
		f = fopen(history.fname, "w");
		if (f) {
			int i;

			fprintf(f, "%s\n", newfile);
			for (i = 0; i < history.count; i++) {
				// Don't create duplicate entries, so check each one against the newfile
				if (strcmp(newfile, history.history[i])) {
					fprintf(f, "%s\n", history.history[i]);
				}
			}
			fclose(f);

			History_load();
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
char *BoardView::History_trim_filename(char *s, int stops) {

	int l   = strlen(s);
	char *p = s + l - 1;

	while ((stops) && (p > s)) {
		if ((*p == '/') || (*p == '\\')) stops--;
		p--;
	}
	if (!stops) p += 2;

	return p;
}

int BoardView::LoadFile(char *filename) {
	if (filename) {
		char *ext = strrchr(filename, '.');
		for (int i = 0; ext[i]; i++) ext[i] = tolower(ext[i]); // Convert extension to lowercase
		SetLastFileOpenName(filename);
		size_t buffer_size;
		char *buffer = file_as_buffer(&buffer_size, filename);
		if (buffer) {
			BRDFile *file = nullptr;
			if (!strcmp(ext, ".brd")) // Recognize file format using filename extension
				file = new BRDFile(buffer, buffer_size);
			else if (!strcmp(ext, ".bdv"))
				file = new BDVFile(buffer, buffer_size);

			if (file && file->valid) {
				SetFile(file);
				History_prepend_save(filename);
				history_file_has_changed = 1; // used by main to know when to update the window title

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

#pragma region Update Logic
void BoardView::Update() {
	bool open_file        = false;
	char *preset_filename = NULL;

	if (ImGui::IsKeyDown(17) && ImGui::IsKeyPressed('O', false)) {
		open_file = true;
		// the dialog will likely eat our WM_KEYUP message for CTRL and O:
		ImGuiIO &io      = ImGui::GetIO();
		io.KeysDown[17]  = false;
		io.KeysDown['O'] = false;
	}
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open", "Ctrl+O")) {
				open_file = true;
			}

			/// Generate file history - PLD20160618-2028
			{
				int i;
				for (i = 0; i < history.count; i++) {
					if (ImGui::MenuItem(History_trim_filename(history.history[i], 2))) {
						open_file       = true;
						preset_filename = history.history[i];
					}
				}
			}

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
		if (ImGui::Button("Net")) {
			m_showNetfilterSearch = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Component")) {
			m_showComponentSearch = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("About")) {
			ImGui::OpenPopup("About");
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
		if (ImGui::BeginPopupModal("Search for Net", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
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
			if (ImGui::IsKeyPressed(13)) {
				SetNetFilter(first_button);
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Button("Clear") || ImGui::IsKeyPressed(27)) {
				SetNetFilter("");
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		if (ImGui::BeginPopupModal("Search for Component", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			if (m_showComponentSearch) {
				m_showComponentSearch = false;
			}
			if (ImGui::InputText("##search", m_search, 128)) {
				FindComponent(m_search);
			}
			const char *first_button = m_search;
			if (ImGui::IsItemHovered() ||
			    (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)))
				ImGui::SetKeyboardFocusHere(-1);
			int buttons_left = 10;
			for (int i = 0; buttons_left && i < m_file->num_parts; i++) {
				const BRDPart &part = m_file->parts[i];
				if (utf8casestr(part.name, m_search)) {
					if (ImGui::Button(part.name)) {
						FindComponent(part.name);
						ImGui::CloseCurrentPopup();
					}
					if (buttons_left == 10) {
						first_button = part.name;
					}
					buttons_left--;
				}
			}
			// Enter and Esc close the search:
			if (ImGui::IsKeyPressed(13)) {
				FindComponent(first_button);
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Button("Clear") || ImGui::IsKeyPressed(27)) {
				FindComponent("");
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("OpenBoardView");
			ImGui::Text("https://github.com/chloridite/OpenBoardView");
			if (ImGui::Button("Close") || ImGui::IsKeyPressed(27)) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::Indent();
			ImGui::Text("License info");
			ImGui::Unindent();
			ImGui::Separator();
			ImGui::Text("OpenBoardView is MIT Licensed");
			ImGui::Text("Copyright (c) 2016 Chloridite and OpenBoardView contributors");
			ImGui::Spacing();
			ImGui::Text("ImGui is MIT Licensed");
			ImGui::Text("Copyright (c) 2014-2015 Omar Cornut and ImGui contributors");
			ImGui::Separator();
			ImGui::Text("The MIT License");
			ImGui::TextWrapped(
			    "Permission is hereby granted, free of charge, to any person obtaining a copy of "
			    "this software and associated documentation files (the \"Software\"), to deal in "
			    "the Software without restriction, including without limitation the rights to use, "
			    "copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the "
			    "Software, and to permit persons to whom the Software is furnished to do so, "
			    "subject to the following conditions: ");
			ImGui::TextWrapped(
			    "The above copyright notice and this permission notice shall be included in all "
			    "copies or substantial portions of the Software.");
			ImGui::TextWrapped(
			    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
			    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS "
			    "FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR "
			    "COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
			    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, "
			    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE "
			    "SOFTWARE.");
			ImGui::EndPopup();
		}
		if (ImGui::BeginPopupModal("Error opening file")) {
			ImGui::Text("There was an error opening the file: %s", m_lastFileOpenName);
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
		}

		if (filename) {
			LoadFile(filename);
		}
	}

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
	                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
	ImGuiWindowFlags draw_surface_flags = flags | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
	ImGui::SetNextWindowPos(ImVec2{0, 0});
	const ImGuiIO &io = ImGui::GetIO();
	if (io.DisplaySize.x != m_lastWidth || io.DisplaySize.y != m_lastHeight) {
		m_lastWidth   = io.DisplaySize.x;
		m_lastHeight  = io.DisplaySize.y;
		m_needsRedraw = true;
	}
	ImGui::SetNextWindowSize(io.DisplaySize);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	if (m_firstFrame) {
		ImGui::SetNextWindowFocus();
		m_firstFrame = false;
	}
	ImGui::Begin("surface", nullptr, draw_surface_flags);
	HandleInput();
	DrawBoard();
	ImGui::End();
	ImGui::PopStyleColor();

	// Overlay
	RenderOverlay();

	// Status Footer
	float status_height = (10.0f + ImGui::GetFontSize());

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 3.0f));
	ImGui::SetNextWindowPos(ImVec2{0, io.DisplaySize.y - status_height});
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, status_height));
	ImGui::Begin("status", nullptr, flags);
	if (m_file && m_board && m_pinSelected) {
		auto pin = m_pinSelected;
		ImGui::Text("Part: %s   Pin: %d   Net: %s   Probe: %d   (%s.)",
		            pin->component->name.c_str(),
		            pin->number,
		            pin->net->name.c_str(),
		            pin->net->number,
		            pin->component->mount_type_str().c_str());
	} else {
		ImVec2 spos = ImGui::GetMousePos();
		ImVec2 pos  = ScreenToCoord(spos.x, spos.y);
		ImGui::Text("Position: %0.3f\", %0.3f\" (%0.2f, %0.2f)mm", pos.x / 1000, pos.y / 1000, pos.x * 0.0254, pos.y * 0.0254);
	}
	ImGui::End();
	ImGui::PopStyleVar();

	ImGui::PopStyleVar();
}

void BoardView::Zoom(float osd_x, float osd_y, float zoom) {
	ImVec2 target;
	ImVec2 coord;

	target.x = osd_x;
	target.y = osd_y;
	coord    = ScreenToCoord(target.x, target.y);

	// Adjust the scale of the whole view, then get the new coordinates ( as CoordToScreen utilises m_scale )
	m_scale        = m_scale * powf(2.0f, zoom);
	ImVec2 dtarget = CoordToScreen(coord.x, coord.y);

	ImVec2 td = ScreenToCoord(target.x - dtarget.x, target.y - dtarget.y, 0);
	m_dx += td.x;
	m_dy += td.y;
	m_needsRedraw = true;
}

void BoardView::HandleInput() {
	const ImGuiIO &io = ImGui::GetIO();
	if (ImGui::IsWindowFocused()) {
		// Pan:
		if (ImGui::IsMouseDragging()) {
			ImVec2 delta = ImGui::GetMouseDragDelta();
			if ((abs(delta.x) > 500) || (abs(delta.y) > 500)) {
				delta.x = 0;
				delta.y = 0;
			} // If the delta values are crazy just drop them (happens when panning off screen). 500 arbritary chosen
			ImGui::ResetMouseDragDelta();
			ImVec2 td = ScreenToCoord(delta.x, delta.y, 0);
			m_dx += td.x;
			m_dy += td.y;
			m_draggingLastFrame = true;
			m_needsRedraw       = true;
		} else {
			// Click to select pin:
			if (m_file && m_board && ImGui::IsMouseReleased(0) && !m_draggingLastFrame) {
				ImVec2 spos = ImGui::GetMousePos();
				ImVec2 pos  = ScreenToCoord(spos.x, spos.y);
				// threshold to within a pin's diameter of the pin center
				float min_dist = m_pinDiameter * 1.0f;
				min_dist *= min_dist; // all distance squared
				Pin *selection = nullptr;
				for (auto &pin : m_board->Pins()) {
					float dx   = pin->position.x - pos.x;
					float dy   = pin->position.y - pos.y;
					float dist = dx * dx + dy * dy;
					if (dist < min_dist) {
						selection = pin.get();
						min_dist  = dist;
					}
				}
				m_pinSelected = selection;
				m_needsRedraw = true;
			}
			m_draggingLastFrame = false;
		}

		// Zoom:
		float mwheel = io.MouseWheel;
		if (mwheel != 0.0f) {
			mwheel *= 0.5f;

			// Ctrl slows down the zoom speed:
			if (ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL) || ImGui::IsKeyDown(17)) {
				mwheel *= 0.1f;
			}
			Zoom(io.MousePos.x, io.MousePos.y, mwheel);
		}
	}
	if (!io.WantCaptureKeyboard) {
		// Flip board:
		if (ImGui::IsKeyPressed(' ')) {
			FlipBoard();
		}

		// Rotate board: R and period rotate clockwise; comma rotates
		// counter-clockwise
		if (ImGui::IsKeyPressed(SDL_SCANCODE_KP_0) || ImGui::IsKeyPressed('R') || ImGui::IsKeyPressed('r')) {
			Rotate(1);
		}
		if (ImGui::IsKeyPressed(SDL_SCANCODE_KP_PERIOD)) {
			Rotate(-1);
		}

		//	 if (ImGui::IsKeyPressed(SDL_SCANCODE_KP_0)) {
		// Use this as a debug button
		//		  fprintf(stderr,"m_dxy: %f %f, m_mxy: %f %f, board: %d %d, lastW/H: %f %f, scale: %f\n", m_dx, m_dy, m_mx, m_my,
		// m_boardWidth, m_boardHeight, m_lastWidth, m_lastHeight, m_scale);
		//	 }

		if (ImGui::IsKeyPressed(SDL_SCANCODE_KP_PLUS)) {
			if (ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL)) {
				Zoom(m_lastWidth / 2, m_lastHeight / 2, 0.01f);
			} else {
				Zoom(m_lastWidth / 2, m_lastHeight / 2, 0.1f);
			}
		}

		if (ImGui::IsKeyPressed(SDL_SCANCODE_KP_MINUS)) {
			if (ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL)) {
				Zoom(m_lastWidth / 2, m_lastHeight / 2, -0.01f);
			} else {
				Zoom(m_lastWidth / 2, m_lastHeight / 2, -0.1f);
			}
		}

		if (ImGui::IsKeyPressed(SDL_SCANCODE_KP_2)) {
			if (ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL)) {
				if (m_current_side)
					m_dy -= 10;
				else
					m_dy += 10;
			} else {
				if (m_current_side)
					m_dy -= 100;
				else
					m_dy += 100;
			}
			m_draggingLastFrame = true;
			m_needsRedraw       = true;
		}

		if (ImGui::IsKeyPressed(SDL_SCANCODE_KP_8)) {
			if (ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL)) {
				if (m_current_side)
					m_dy += 10;
				else
					m_dy -= 10;
			} else {
				if (m_current_side)
					m_dy += 100;
				else
					m_dy -= 100;
			}
			m_draggingLastFrame = true;
			m_needsRedraw       = true;
		}

		if (ImGui::IsKeyPressed(SDL_SCANCODE_KP_4)) {
			if (ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL))
				m_dx += 10;
			else
				m_dx += 100;
			m_draggingLastFrame = true;
			m_needsRedraw       = true;
		}

		if (ImGui::IsKeyPressed(SDL_SCANCODE_KP_6)) {
			if (ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL))
				m_dx -= 10;
			else
				m_dx -= 100;
			m_draggingLastFrame = true;
			m_needsRedraw       = true;
		}

		// Center and reset zoom
		if (ImGui::IsKeyPressed('5') || ImGui::IsKeyPressed(SDL_SCANCODE_KP_5)) {
			CenterView();
		}

		// Search for net
		if (ImGui::IsKeyPressed('N') || ImGui::IsKeyPressed('n')) {
			m_showNetfilterSearch = true;
		}

		// Search for component
		if (ImGui::IsKeyPressed('C') || ImGui::IsKeyPressed('c')) {
			m_showComponentSearch = true;
		}

		// Show Net List
		if (ImGui::IsKeyPressed('L') || ImGui::IsKeyPressed('l')) {
			m_showNetList = m_showNetList ? false : true;
		}

		// Show Part List
		if (ImGui::IsKeyPressed('K') || ImGui::IsKeyPressed('k')) {
			m_showPartList = m_showPartList ? false : true;
		}
	}
}
#pragma endregion

#pragma region Overlay &Windows
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
#pragma endregion Showing UI floating above main workspace.

#pragma region Drawing
inline void BoardView::DrawOutline(ImDrawList *draw) {
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

		// if we encounter our hull/poly start point, then we've now created the closed
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
		draw->AddLine(spa, spb, m_colors.boardOutline);
	} // for
}

inline void BoardView::DrawPins(ImDrawList *draw) {
	auto io = ImGui::GetIO();

	for (auto &pin : m_board->Pins()) {
		auto p_pin = pin.get();
		float psz  = pin->diameter * m_scale * 20.0f;

		// continue if pin is not visible anyway
		ImVec2 pos = CoordToScreen(pin->position.x, pin->position.y);
		{
			if (!ElementIsVisible(p_pin)) continue;

			if (!IsVisibleScreen(pos.x, pos.y, psz, io)) continue;
		}

		// color & text depending on app state & pin type
		uint32_t color      = m_colors.pinDefault;
		uint32_t text_color = color;
		bool show_text      = false;
		{
			if (contains(*pin, m_pinHighlighted)) {
				text_color = color = m_colors.pinHighlighted;
				show_text          = true;
			}

			if (!pin->net || pin->type == Pin::kPinTypeNotConnected) {
				color = m_colors.pinNotConnected;
			} else {
				if (pin->net->is_ground) color = m_colors.pinGround;
			}

			if (PartIsHighlighted(*pin->component)) {
				if (!show_text) {
					// TODO: not sure how to name this
					color      = 0xffff8000;
					text_color = 0xff808000;
				}
				show_text = true;
			}

			if (pin->type == Pin::kPinTypeTestPad) {
				color     = m_colors.pinTestPad;
				show_text = false;
			}

			// pin is on the same net as selected pin: highlight > rest
			if (!show_text && m_pinSelected && pin->net == m_pinSelected->net) {
				color = m_colors.pinHighlightSameNet;
			}

			// pin selected overwrites everything
			if (p_pin == m_pinSelected) {
				color      = m_colors.pinSelected;
				text_color = m_colors.pinSelected;
				show_text  = true;
			}

			// don't show text if it doesn't make sense
			if (pin->component->pins.size() <= 1 || pin->type == Pin::kPinTypeTestPad) show_text = false;
		}

		// Drawing
		{
			char pin_number[64];
			int segments;
			draw->ChannelsSetCurrent(kChannelImages);

			// for the round pin representations, choose how many circle segments need based on the pin size
			segments                    = trunc(psz);
			if (segments < 8) segments  = 8;
			if (segments > 32) segments = 32;

			switch (pin->type) {
				case Pin::kPinTypeTestPad: draw->AddCircleFilled(ImVec2(pos.x, pos.y), psz, color, segments); break;
				default: draw->AddCircle(ImVec2(pos.x, pos.y), psz, color, segments);
			}

			if (show_text) {
				snprintf(pin_number, sizeof(pin_number), "%d", pin->number);

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

void BoardView::RotateV(double *px, double *py, double ox, double oy, double theta) {
	double tx, ty, ttx, tty;

	tx = *px - ox;
	ty = *py - oy;

	// With not a lot of parts on the boards, we can get away with using the precision trig functions, might have to change to LUT
	// based later.
	ttx = tx * cos(theta) - ty * sin(theta);
	tty = tx * sin(theta) + ty * cos(theta);

	*px = ttx + ox;
	*py = tty + oy;
}

ImVec2 BoardView::RotateV(ImVec2 v, ImVec2 o, double theta) {
	double tx, ty, ttx, tty;

	tx = v.x - o.x;
	ty = v.y - o.y;

	// With not a lot of parts on the boards, we can get away with using the precision trig functions, might have to change to LUT
	// based later.
	ttx = tx * cos(theta) - ty * sin(theta);
	tty = tx * sin(theta) + ty * cos(theta);

	return ImVec2(ttx + o.x, tty + o.y);
}

ImVec2 BoardView::RotateV(ImVec2 v, double theta) {
	double nx, ny;

	nx = v.x * cos(theta) - v.y * sin(theta);
	ny = v.x * sin(theta) + v.y * cos(theta);

	return ImVec2(nx, ny);
}

double BoardView::AngleToX(ImVec2 a, ImVec2 b) {
	return atan2((b.y - a.y), (b.x - a.x));
}

void BoardView::MBBCalculate(ImVec2 box[], ImVec2 *hull, int n, double psz) {

	double mbAngle = 0, cumulative_angle = 0;
	double mbArea = DBL_MAX; // fake area to initialise
	ImVec2 mbb, mba, origin; // Box bottom left, box top right
	int i;

	origin = hull[0];

	for (i = 0; i < n; i++) {
		int ni = i + 1;
		double area;

		ImVec2 current = hull[i];
		ImVec2 next    = hull[ni % n];

		double angle = AngleToX(current, next); // angle formed between current and next hull points;
		cumulative_angle += angle;

		double top, bot, left, right; // bounding rect limits
		top = right = DBL_MIN;
		bot = left = DBL_MAX;

		int x;
		for (x = 0; x < n; x++) {
			ImVec2 rp = RotateV(hull[x], origin, -angle);

			hull[x] = rp;

			if (rp.y > top) top     = rp.y;
			if (rp.y < bot) bot     = rp.y;
			if (rp.x > right) right = rp.x;
			if (rp.x < left) left   = rp.x;
		}
		area = (right - left) * (top - bot);

		if (area < mbArea) {
			mbArea  = area;
			mbAngle = cumulative_angle; // total angle we've had to rotate the board to get to this orientation;
			mba     = ImVec2(left, bot);
			mbb     = ImVec2(right, top);
		}
	} // for all points on hull

	// expand by pin size
	mba.x -= psz;
	mba.y -= psz;
	mbb.x += psz;
	mbb.y += psz;

	// Form our rectangle, has to be all 4 points as it's a polygon now that'll be rotated
	box[0] = RotateV(mba, origin, +mbAngle);
	box[1] = RotateV(ImVec2(mbb.x, mba.y), origin, +mbAngle);
	box[2] = RotateV(mbb, origin, +mbAngle);
	box[3] = RotateV(ImVec2(mba.x, mbb.y), origin, +mbAngle);
}

// To find orientation of ordered triplet (p, q, r).
// The function returns following values
// 0 --> p, q and r are colinear
// 1 --> Clockwise
// 2 --> Counterclockwise
int BoardView::ConvexHullOrientation(ImVec2 p, ImVec2 q, ImVec2 r) {

	int val = trunc(((q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y)));

	if (val == 0) return 0;   // colinear
	return (val > 0) ? 1 : 2; // clock or counterclock wise
}

int BoardView::ConvexHull(ImVec2 hull[], ImVec2 points[], int n) {

	int hpc = 0;

	// There must be at least 3 points
	if (n < 3) return 0;

	// Find the leftmost point
	int l = 0;
	for (int i = 1; i < n; i++) {
		if (points[i].x < points[l].x) {
			l = i;
		}
	}

	// Start from leftmost point, keep moving counterclockwise
	// until reach the start point again.  This loop runs O(h)
	// times where h is number of points in result or output.
	int p = l, q;
	do {
		// Add current point to result
		hull[hpc] = CoordToScreen(points[p].x, points[p].y);
		hpc++;

		// Search for a point 'q' such that orientation(p, x,
		// q) is counterclockwise for all points 'x'. The idea
		// is to keep track of last visited most counterclock-
		// wise point in q. If any point 'i' is more counterclock-
		// wise than q, then update q.
		q = (p + 1) % n;
		for (int i = 0; i < n; i++) {
			// If i is more counterclockwise than current q, then update q
			if (ConvexHullOrientation(points[p], points[i], points[q]) == 2) {
				q = i;
			}
		}

		// Now q is the most counterclockwise with respect to p
		// Set p as q for next iteration, so that q is added to
		// result 'hull'
		p = q;

	} while ((p != l) && (hpc < n)); // While we don't come to first point

	return hpc;
}

int BoardView::TightenHull(ImVec2 hull[], int n, double threshold) {
	// theory: circle the hull, compare 3 points at a time, if the mid point is sub-angular then make it equal the first point and
	// move to the 3rd.
	int i, ni;
	ImVec2 *a, *b, *c;
	double a1, a2, ad;
	// First cycle, we look for sub-threshold 2-segment runs
	for (i = 0; i < n; i++) {
		a = &(hull[i]);
		b = &(hull[(i + 1) % n]);
		c = &(hull[(i + 2) % n]);

		a1 = AngleToX(*a, *b);
		a2 = AngleToX(*b, *c);
		if (a1 > a2)
			ad = a1 - a2;
		else
			ad = a2 - a1;

		if (ad < threshold) {
			//			fprintf(stderr,"angle below threshold (%0.2f)\n", ad);
			*b = *a;
		}
	} // end first cycle

	// Second cycle, we compact the hull
	int output_index = 0;
	i                = 0;
	while (i < n) {
		ni = (i + 1) % n;
		if ((hull[i].x == hull[ni].x) && (hull[i].y == hull[ni].y)) {
			// match found, discard one
			i++;
			continue;
		}

		hull[output_index] = hull[i];
		output_index++;
		i++;
	}

	return output_index;
}

inline void BoardView::DrawParts(ImDrawList *draw) {
	float psz = (float)m_pinDiameter * 0.5f * m_scale;
	double angle;
	double distance = 0;
	struct ImVec2 pva[1000], *ppp;
	uint32_t color = m_colors.boxColor;

	for (auto &part : m_board->Components()) {
		int pincount           = 0;
		int part_is_orthagonal = 0;
		double min_x, min_y, max_x, max_y;
		auto p_part = part.get();

		if (!ElementIsVisible(p_part)) continue;

		if (part->is_dummy()) continue;

		ppp = &pva[0];
		for (auto pin : part->pins) {
			pincount++;

			// scale box around pins as a fallback, else either use polygon or convex hull for better shape fidelity
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

			if (pin->position.x > max_x)
				max_x = pin->position.x;
			else if (pin->position.x < min_x)
				min_x = pin->position.x;
			if (pin->position.y > max_y)
				max_y = pin->position.y;
			else if (pin->position.y < min_y)
				min_y = pin->position.y;
		}

		if (min_x < max_x)
			distance = max_y - min_y;
		else
			distance = max_x - min_x;

		// Test to see if the part is strictly horizontal or vertical
		part_is_orthagonal = 1;
		if ((min_y < max_y) && (min_x < max_x)) {
			if (((max_y - min_y) > 2) && ((max_x - min_x) > 2)) {
				part_is_orthagonal = 0;
			}
		}
		distance = sqrt((max_x - min_x) * (max_x - min_x) + (max_y - min_y) * (max_y - min_y));

		float pin_radius = m_pinDiameter / 2.0f;

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
		min_x -= pin_radius;
		max_x += pin_radius;
		min_y -= pin_radius;
		max_y += pin_radius;
		bool bb_y_resized = false;

		// If the part is prefixed with 'U', meaning an IC, then retract the box inwards
		/* -- commented out by Inflex, there's as much chance the IC legs out vs BGA, so
		    really this could go either way

		   if (p_part->name[0] == 'U') {
		       if (min_x < max_x - 4 * m_pinDiameter) {
		   min_x += m_pinDiameter/2;
		   max_x -= m_pinDiameter/2;
		       }
		       if (min_y < max_y - 4 * m_pinDiameter) {
		   min_y += m_pinDiameter/2;
		   max_y -= m_pinDiameter/2;
		           bb_y_resized = true;
		       }
		   }
		*/

		ImVec2 min = CoordToScreen(min_x, min_y);
		ImVec2 max = CoordToScreen(max_x, max_y);
		//    min.x -= 0.5f;
		//  max.x += 0.5f;

		// If we have a rotated part that is a C or R, hopefully with 2 pins ?
		if ((!part_is_orthagonal) && (pincount < 3) &&
		    (((p_part->name[0] == 'C') || (p_part->name[0] == 'R') || (p_part->name[0] == 'L') || (p_part->name[0] == 'D')))) {
			ImVec2 a, b, c, d;
			double dx, dy;
			double tx, ty;
			double arm;

			dx    = part->pins[1]->position.x - part->pins[0]->position.x;
			dy    = part->pins[1]->position.y - part->pins[0]->position.y;
			angle = atan2(dy, dx);

			arm = pin_radius;

			// TODO: Compact this bit of code, maybe. It works at least.
			tx = part->pins[0]->position.x - arm;
			ty = part->pins[0]->position.y - arm;
			RotateV(&tx, &ty, part->pins[0]->position.x, part->pins[0]->position.y, angle);
			a = CoordToScreen(tx, ty);

			tx = part->pins[0]->position.x - arm;
			ty = part->pins[0]->position.y + arm;
			RotateV(&tx, &ty, part->pins[0]->position.x, part->pins[0]->position.y, angle);
			b = CoordToScreen(tx, ty);

			tx = part->pins[1]->position.x + arm;
			ty = part->pins[1]->position.y + arm;
			RotateV(&tx, &ty, part->pins[1]->position.x, part->pins[1]->position.y, angle);
			c = CoordToScreen(tx, ty);

			tx = part->pins[1]->position.x + arm;
			ty = part->pins[1]->position.y - arm;
			RotateV(&tx, &ty, part->pins[1]->position.x, part->pins[1]->position.y, angle);
			d = CoordToScreen(tx, ty);
			draw->AddQuad(a, b, c, d, color);

		} else {

			// If we have (typically) a connector with a non uniform pin distribution
			// then we can try use the minimal bounding box algorithm to give it a
			// more sane outline
			if ((pincount > 4) && ((part->name[0] == 'J') || (strncmp(part->name.c_str(), "CN", 2) == 0))) {
				ImVec2 *hull;
				int hpc;

				hull = (ImVec2 *)malloc(sizeof(ImVec2) *
				                        pincount); // massive overkill since our hull will only require the perimeter points
				if (hull) {
					hpc = ConvexHull(hull, pva, pincount); // (hpc = hull pin count)
					if (hpc > 0) {
						ImVec2 bbox[4];

						// first, tighten the hull, removes any small angle segments, such as a sequence of pins in a line
						hpc = TightenHull(hull, hpc, 0.1f); // tighten the hull a bit more, this might be an overkill
						MBBCalculate(bbox, hull, hpc, pin_radius * m_scale);
						draw->AddPolyline(bbox, 4, color, true, 1.0f, true);
					} else {
						draw->AddRect(min, max, color);
					}

					free(hull);
				} else {
					fprintf(stderr, "ERROR: Cannot allocate memory for convex hull generation (%s)", strerror(errno));
					draw->AddRect(min, max, color);
				}

			} else {
				draw->AddRect(min, max, color);
			}
		}

		if (PartIsHighlighted(*part) && !part->is_dummy() && !part->name.empty()) {
			ImVec2 text_size         = ImGui::CalcTextSize(part->name.c_str());
			float top_y              = min.y;
			if (max.y < top_y) top_y = max.y;
			ImVec2 pos               = ImVec2((min.x + max.x) * 0.5f, top_y);
			if (bb_y_resized) {
				pos.y -= text_size.y + 2.0f * psz;
			} else {
				pos.y -= text_size.y;
			}
			pos.x -= text_size.x * 0.5f;
			draw->ChannelsSetCurrent(kChannelPolylines);
			draw->AddRectFilled(ImVec2(pos.x - 2.0f, pos.y - 1.0f),
			                    ImVec2(pos.x + text_size.x + 2.0f, pos.y + text_size.y + 1.0f),
			                    m_colors.backgroundColor,
			                    3.0f);
			draw->ChannelsSetCurrent(kChannelText);
			draw->AddText(pos, m_colors.partTextColor, part->name.c_str());
		}
	}
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

	draw->ChannelsMerge();

	// Copy the new draw list and cmd buffer:
	memcpy(m_cachedDrawList, draw, sizeof(ImDrawList));
	int cmds_size = draw->CmdBuffer.size() * sizeof(ImDrawCmd);
	m_cachedDrawCommands.resize(cmds_size);
	memcpy(m_cachedDrawCommands.Data, draw->CmdBuffer.Data, cmds_size);
	m_needsRedraw = false;
}
#pragma endregion

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

	int min_x = 1000000, max_x = 0, min_y = 1000000, max_y = 0;
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

inline bool BoardView::ElementIsVisible(const BoardElement *element) {
	if (!element) return true; // no component? => no board side info

	if (element->board_side == kBoardSideBoth) return true;

	if (element->board_side == m_current_side) return true;

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
					any_visible |= ElementIsVisible(pin->component);
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

void BoardView::FindComponent(const char *name) {
	if (!m_file || !m_board) return;

	m_pinHighlighted.clear();
	m_partHighlighted.clear();

	string comp_name = string(name);

	if (!comp_name.empty()) {
		Component *part_found = nullptr;
		bool any_visible      = false;

		for (auto &component : m_board->Components()) {
			if (is_prefix(comp_name, component->name)) {
				auto p = component.get();
				m_partHighlighted.push_back(p);
				any_visible |= ElementIsVisible(p);
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
