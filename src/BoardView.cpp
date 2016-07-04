#define _CRT_SECURE_NO_WARNINGS 1
#include "BoardView.h"

#include <algorithm>
#include <math.h>
#include <memory>
#include <stdio.h>

#include "BRDBoard.h"
#include "BRDFile.h"
#include "imgui.h"

#include "NetList.h"
#include "PartList.h"

#include "platform.h"

using namespace std;
using namespace std::placeholders;

#if _MSC_VER
#define stricmp _stricmp
#endif

BoardView::~BoardView() {
	delete m_file;
	delete m_board;
	free(m_lastFileOpenName);
}

#pragma region Update Logic
void BoardView::Update() {
	bool open_file = false;
	if (ImGui::IsKeyDown(17) && ImGui::IsKeyPressed('O', false)) {
		open_file = true;
		// the dialog will likely eat our WM_KEYUP message for CTRL and O:
		ImGuiIO &io = ImGui::GetIO();
		io.KeysDown[17] = false;
		io.KeysDown['O'] = false;
	}
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open", "Ctrl+O")) {
				open_file = true;
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
			if (ImGui::MenuItem("Rotate CW", ".")) {
				Rotate(1);
			}
			if (ImGui::MenuItem("Rotate CCW", ",")) {
				Rotate(-1);
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
			if (ImGui::IsItemHovered() || (ImGui::IsRootWindowOrAnyChildFocused() &&
			                               !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)))
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
		if (ImGui::BeginPopupModal("Search for Component", nullptr,
		                           ImGuiWindowFlags_AlwaysAutoResize)) {
			if (m_showComponentSearch) {
				m_showComponentSearch = false;
			}
			if (ImGui::InputText("##search", m_search, 128)) {
				FindComponent(m_search);
			}
			const char *first_button = m_search;
			if (ImGui::IsItemHovered() || (ImGui::IsRootWindowOrAnyChildFocused() &&
			                               !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)))
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
		char *filename = show_file_picker();
		if (filename) {
			SetLastFileOpenName(filename);
			size_t buffer_size;
			char *buffer = file_as_buffer(&buffer_size, filename);
			if (buffer) {
				BRDFile *file = new BRDFile(buffer, buffer_size);
				if (file->valid) {
					SetFile(file);
				} else {
					m_lastFileOpenWasInvalid = true;
					delete file;
				}
				free(buffer);
			}
		}
	}

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
	                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
	                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
	ImGuiWindowFlags draw_surface_flags =
	    flags | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
	ImGui::SetNextWindowPos(ImVec2{0, 0});
	const ImGuiIO &io = ImGui::GetIO();
	if (io.DisplaySize.x != m_lastWidth || io.DisplaySize.y != m_lastHeight) {
		m_lastWidth = io.DisplaySize.x;
		m_lastHeight = io.DisplaySize.y;
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
		            pin->component->name.c_str(), pin->number, pin->net->name.c_str(),
		            pin->net->number, pin->component->mount_type_str().c_str());
	}
	ImGui::End();
	ImGui::PopStyleVar();

	ImGui::PopStyleVar();
}

void BoardView::HandleInput() {
	const ImGuiIO &io = ImGui::GetIO();
	if (ImGui::IsWindowFocused()) {
		// Pan:
		if (ImGui::IsMouseDragging()) {
			ImVec2 delta = ImGui::GetMouseDragDelta();
			ImGui::ResetMouseDragDelta();
			ImVec2 td = ScreenToCoord(delta.x, delta.y, 0);
			m_dx += td.x;
			m_dy += td.y;
			m_draggingLastFrame = true;
			m_needsRedraw = true;
		} else {
			// Click to select pin:
			if (m_file && m_board && ImGui::IsMouseReleased(0) && !m_draggingLastFrame) {
				ImVec2 spos = ImGui::GetMousePos();
				ImVec2 pos = ScreenToCoord(spos.x, spos.y);
				// threshold to within a pin's diameter of the pin center
				float min_dist = m_pinDiameter * 1.0f;
				min_dist *= min_dist; // all distance squared
				Pin *selection = nullptr;
				for (auto &pin : m_board->Pins()) {
					float dx = pin->position.x - pos.x;
					float dy = pin->position.y - pos.y;
					float dist = dx * dx + dy * dy;
					if (dist < min_dist) {
						selection = pin.get();
						min_dist = dist;
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
			const ImVec2 &target = io.MousePos;
			ImVec2 coord = ScreenToCoord(target.x, target.y);
			mwheel *= 0.5f;
			// Ctrl slows down the zoom speed:
			if (ImGui::IsKeyDown(17)) {
				mwheel *= 0.1f;
			}
			m_scale = m_scale * powf(2.0f, mwheel);

			ImVec2 dtarget = CoordToScreen(coord.x, coord.y);
			ImVec2 td = ScreenToCoord(target.x - dtarget.x, target.y - dtarget.y, 0);
			m_dx += td.x;
			m_dy += td.y;
			m_needsRedraw = true;
		}
	}
	if (!io.WantCaptureKeyboard) {
		// Flip board:
		if (ImGui::IsKeyPressed(' ')) {
			FlipBoard();
		}

		// Rotate board: R and period rotate clockwise; comma rotates
		// counter-clockwise
		if (ImGui::IsKeyPressed('R') || ImGui::IsKeyPressed(190)) {
			Rotate(1);
		}
		if (ImGui::IsKeyPressed(188)) {
			Rotate(-1);
		}

		// Search for net
		if (ImGui::IsKeyPressed('N')) {
			m_showNetfilterSearch = true;
		}

		// Search for component
		if (ImGui::IsKeyPressed('C')) {
			m_showComponentSearch = true;
		}

		// Show Net List
		if (ImGui::IsKeyPressed('L')) {
			m_showNetList = m_showNetList ? false : true;
		}

		// Show Part List
		if (ImGui::IsKeyPressed('K')) {
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
	auto &outline = m_board->OutlinePoints();

	for (int i = 0; i < outline.size() - 1; i++) {
		Point &pa = *outline[i];
		Point &pb = *outline[i + 1];
		if (pa.x == pb.x && pa.y == pb.y)
			continue;
		ImVec2 spa = CoordToScreen(pa.x, pa.y);
		ImVec2 spb = CoordToScreen(pb.x, pb.y);
		draw->AddLine(spa, spb, m_colors.boardOutline);
	}
}

inline void BoardView::DrawPins(ImDrawList *draw) {
	ImTextureID filled_circle_tex = TextureIDs[0];
	ImTextureID empty_circle_tex = TextureIDs[1];

	// TODO: use pin->diameter
	float psz = (float)m_pinDiameter * 0.5f * m_scale;

	auto io = ImGui::GetIO();

	for (auto &pin : m_board->Pins()) {
		auto p_pin = pin.get();

		// continue if pin is not visible anyway
		ImVec2 pos = CoordToScreen(pin->position.x, pin->position.y);
		{
			if (!ElementIsVisible(p_pin))
				continue;

			if (!IsVisibleScreen(pos.x, pos.y, psz, io))
				continue;
		}

		// color & text depending on app state & pin type
		auto pin_texture = empty_circle_tex;
		uint32_t color = m_colors.pinDefault;
		uint32_t text_color = color;
		bool show_text = false;
		{
			if (contains(*pin, m_pinHighlighted)) {
				text_color = color = m_colors.pinHighlighted;
				show_text = true;
			}

			if (!pin->net || pin->type == Pin::kPinTypeNotConnected) {
				color = m_colors.pinNotConnected;
			} else {
				if (pin->net->is_ground)
					color = m_colors.pinGround;
			}

			if (PartIsHighlighted(*pin->component)) {
				if (!show_text) {
					// TODO: not sure how to name this
					color = 0xffff8000;
					text_color = 0xff808000;
				}
				show_text = true;
			}

			if (pin->type == Pin::kPinTypeTestPad) {
				color = m_colors.pinTestPad;
				// pin_texture = filled_circle_tex; // TODO
				show_text = false;
			}

			// pin is on the same net as selected pin: highlight > rest
			if (!show_text && m_pinSelected && pin->net == m_pinSelected->net) {
				color = m_colors.pinHighlightSameNet;
			}

			// pin selected overwrites everything
			if (p_pin == m_pinSelected) {
				color = m_colors.pinSelected;
				text_color = m_colors.pinSelected;
				show_text = true;
			}

			// don't show text if it doesn't make sense
			if (pin->component->pins.size() <= 1 || pin->type == Pin::kPinTypeTestPad)
				show_text = false;
		}

		// Drawing
		{
			char pin_number[64];
			draw->ChannelsSetCurrent(kChannelImages);
			draw->AddImage(pin_texture, ImVec2(pos.x - psz, pos.y - psz),
			               ImVec2(pos.x + psz, pos.y + psz), ImVec2(0, 0), ImVec2(1, 1), color);
			if (show_text) {
				sprintf(pin_number, "%d", pin->number);

				ImVec2 text_size = ImGui::CalcTextSize(pin_number);
				ImVec2 pos_adj = ImVec2(pos.x - text_size.x * 0.5f, pos.y - text_size.y * 0.5f);
				draw->ChannelsSetCurrent(kChannelPolylines);
				draw->AddRectFilled(
				    ImVec2(pos_adj.x - 2.0f, pos_adj.y - 1.0f),
				    ImVec2(pos_adj.x + text_size.x + 2.0f, pos_adj.y + text_size.y + 1.0f),
				    m_colors.backgroundColor, 3.0f);
				draw->ChannelsSetCurrent(kChannelText);
				draw->AddText(pos_adj, text_color, pin_number);
			}
		}
	}
}

inline void BoardView::DrawParts(ImDrawList *draw) {
	float psz = (float)m_pinDiameter * 0.5f * m_scale;

	for (auto &part : m_board->Components()) {
		auto p_part = part.get();

		if (!ElementIsVisible(p_part))
			continue;

		if (part->is_dummy())
			continue;

		// scale box around pins
		float min_x = part->pins[0]->position.x;
		float min_y = part->pins[0]->position.y;
		float max_x = min_x;
		float max_y = min_y;
		for (auto pin : part->pins) {
			if (pin->position.x > max_x)
				max_x = pin->position.x;
			else if (pin->position.x < min_x)
				min_x = pin->position.x;
			if (pin->position.y > max_y)
				max_y = pin->position.y;
			else if (pin->position.y < min_y)
				min_y = pin->position.y;
		}

		// TODO: pin radius is stored in Pin object
		float pin_radius = m_pinDiameter / 2.0f;
		min_x -= pin_radius;
		max_x += pin_radius;
		min_y -= pin_radius;
		max_y += pin_radius;
		bool bb_y_resized = false;
		if (p_part->name[0] == 'U') {
			if (min_x < max_x - 4 * m_pinDiameter) {
				min_x += m_pinDiameter;
				max_x -= m_pinDiameter;
			}
			if (min_y < max_y - 4 * m_pinDiameter) {
				min_y += m_pinDiameter;
				max_y -= m_pinDiameter;
				bb_y_resized = true;
			}
		}

		uint32_t color = m_colors.boxColor;
		ImVec2 min = CoordToScreen(min_x, min_y);
		ImVec2 max = CoordToScreen(max_x, max_y);
		min.x -= 0.5f;
		max.x += 0.5f;
		draw->AddRect(min, max, color);
		if (PartIsHighlighted(*part) && !part->is_dummy() && !part->name.empty()) {
			ImVec2 text_size = ImGui::CalcTextSize(part->name.c_str());
			float top_y = min.y;
			if (max.y < top_y)
				top_y = max.y;
			ImVec2 pos = ImVec2((min.x + max.x) * 0.5f, top_y);
			if (bb_y_resized) {
				pos.y -= text_size.y + 2.0f * psz;
			} else {
				pos.y -= text_size.y;
			}
			pos.x -= text_size.x * 0.5f;
			draw->ChannelsSetCurrent(kChannelPolylines);
			draw->AddRectFilled(ImVec2(pos.x - 2.0f, pos.y - 1.0f),
			                    ImVec2(pos.x + text_size.x + 2.0f, pos.y + text_size.y + 1.0f),
			                    m_colors.backgroundColor, 3.0f);
			draw->ChannelsSetCurrent(kChannelText);
			draw->AddText(pos, m_colors.partTextColor, part->name.c_str());
		}
	}
}

void BoardView::DrawBoard() {
	if (!m_file || !m_board)
		return;

	ImDrawList *draw = ImGui::GetWindowDrawList();
	if (!m_needsRedraw) {
		memcpy(draw, m_cachedDrawList, sizeof(ImDrawList));
		memcpy(draw->CmdBuffer.Data, m_cachedDrawCommands.Data, m_cachedDrawCommands.Size);
		return;
	}

	// Splitting channels, drawing onto those and merging back.
	draw->ChannelsSplit(NUM_DRAW_CHANNELS);
	draw->ChannelsSetCurrent(kChannelPolylines);

	DrawOutline(draw);
	DrawPins(draw);
	DrawParts(draw);

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

void BoardView::SetFile(BRDFile *file) {
	delete m_file;
	delete m_board;

	m_file = file;
	m_board = new BRDBoard(file);

	m_nets = m_board->Nets();

	int min_x = 1000000, max_x = 0, min_y = 1000000, max_y = 0;
	for (int i = 0; i < m_file->num_format; i++) {
		BRDPoint &pa = m_file->format[i];
		if (pa.x < min_x)
			min_x = pa.x;
		if (pa.y < min_y)
			min_y = pa.y;
		if (pa.x > max_x)
			max_x = pa.x;
		if (pa.y > max_y)
			max_y = pa.y;
	}

	m_mx = (float)(min_x + max_x) / 2.0f;
	m_my = (float)(min_y + max_y) / 2.0f;

	ImVec2 view = ImGui::GetIO().DisplaySize;
	float dx = 1.05f * (max_x - min_x);
	float dy = 1.05f * (max_y - min_y);
	float sx = dx > 0 ? view.x / dx : 1.0f;
	float sy = dy > 0 ? view.y / dy : 1.0f;

	m_scale = sx < sy ? sx : sy;
	m_boardWidth = max_x - min_x;
	m_boardHeight = max_y - min_y;
	SetTarget(m_mx, m_my);

	m_pinHighlighted.reserve(m_board->Components().size());
	m_partHighlighted.reserve(m_board->Components().size());
	m_pinSelected = nullptr;

	m_firstFrame = true;
	m_needsRedraw = true;
}

ImVec2 BoardView::CoordToScreen(float x, float y, float w) {
	float side = m_current_side ? -1.0f : 1.0f;
	float tx = side * m_scale * (x + w * (m_dx - m_mx));
	float ty = -1.0f * m_scale * (y + w * (m_dy - m_my));
	switch (m_rotation) {
	case 0:
		return ImVec2(tx, ty);
	case 1:
		return ImVec2(-ty, tx);
	case 2:
		return ImVec2(-tx, -ty);
	default:
		return ImVec2(ty, -tx);
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
	float side = m_current_side ? -1.0f : 1.0f;
	float invscale = 1.0f / m_scale;

	tx = tx * side * invscale + w * (m_mx - m_dx);
	ty = ty * -1.0f * invscale + w * (m_my - m_dy);

	return ImVec2(tx, ty);
}

void BoardView::Rotate(int count) {
	// too lazy to do math
	while (count > 0) {
		m_rotation = (m_rotation + 1) & 3;
		float dx = m_dx;
		float dy = m_dy;
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
		float dx = m_dx;
		float dy = m_dy;
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
	ImVec2 view = ImGui::GetIO().DisplaySize;
	ImVec2 coord = ScreenToCoord(view.x / 2.0f, view.y / 2.0f);
	m_dx += coord.x - x;
	m_dy += coord.y - y;
}

inline bool BoardView::ElementIsVisible(const BoardElement *element) {
	if (!element)
		return true; // no component? => no board side info

	if (element->board_side == kBoardSideBoth)
		return true;

	if (element->board_side == m_current_side)
		return true;

	return false;
}

inline bool BoardView::IsVisibleScreen(float x, float y, float radius, const ImGuiIO &io) {
	if (x < -radius || y < -radius || x - radius > io.DisplaySize.x ||
	    y - radius > io.DisplaySize.y)
		return false;
	return true;
}

bool BoardView::PartIsHighlighted(const Component &component) {
	bool highlighted = contains(component, m_partHighlighted);

	// is any pin of this part selected?
	if (m_pinSelected)
		highlighted |= m_pinSelected->component == &component;

	return highlighted;
}

void BoardView::SetNetFilter(const char *net) {
	strcpy(m_netFilter, net);
	if (!m_file || !m_board)
		return;

	m_pinHighlighted.clear();
	m_partHighlighted.clear();

	string net_name = string(net);

	if (!net_name.empty()) {
		bool any_visible = false;
		int count = 0;

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
			if (!any_visible)
				FlipBoard();
			m_pinSelected = nullptr;
		}
	}

	m_needsRedraw = true;
}

void BoardView::FindComponent(const char *name) {
	if (!m_file || !m_board)
		return;

	m_pinHighlighted.clear();
	m_partHighlighted.clear();

	string comp_name = string(name);

	if (!comp_name.empty()) {
		Component *part_found = nullptr;
		bool any_visible = false;

		for (auto &component : m_board->Components()) {
			if (is_prefix(comp_name, component->name)) {
				auto p = component.get();
				m_partHighlighted.push_back(p);
				any_visible |= ElementIsVisible(p);
				part_found = p;
			}
		}

		if (part_found) {
			if (!any_visible)
				FlipBoard();
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
		uint32_t bytelen = 4 * ((m_size + 31) / 32);
		uint32_t new_bytelen = 4 * ((new_size + 31) / 32);
		uint32_t *new_bits = (uint32_t *)malloc(new_bytelen);
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
