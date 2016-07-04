#define _CRT_SECURE_NO_WARNINGS 1
#include <math.h>
#include <stdio.h>

#include "BoardView.h"
#include "BRDFile.h"
#include "imgui/imgui.h"
#include "platform.h"
#if _MSC_VER
#define stricmp _stricmp
#endif

BoardView::~BoardView() {
	delete m_file;
	free(m_lastFileOpenName);
}

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
			for (int i = 0; buttons_left && i < m_nets.Size; i++) {
				if (utf8casestr(m_nets[i], m_search)) {
					if (ImGui::Button(m_nets[i])) {
						SetNetFilter(m_nets[i]);
						ImGui::CloseCurrentPopup();
					}
					if (buttons_left == 10) {
						first_button = m_nets[i];
					}
					buttons_left--;
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
			    "Permission is hereby granted, free of charge, to any person obtaining a copy "
			    "of this software and associated documentation files (the \"Software\"), to deal "
			    "in the Software without restriction, including without limitation the rights "
			    "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
			    "copies of the Software, and to permit persons to whom the Software is "
			    "furnished to do so, subject to the following conditions: ");
			ImGui::TextWrapped(
			    "The above copyright notice and this permission notice shall be included in all "
			    "copies or substantial portions of the Software.");
			ImGui::TextWrapped(
			    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
			    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
			    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
			    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
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

	float status_height = (10.0f + ImGui::GetFontSize());

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 3.0f));
	ImGui::SetNextWindowPos(ImVec2{0, io.DisplaySize.y - status_height});
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, status_height));
	ImGui::Begin("status", nullptr, flags);
	if (m_file && m_pinSelected >= 0 && m_pinSelected < m_file->num_pins) {
		const BRDPin &pin = m_file->pins[m_pinSelected];
		const BRDPart &part = m_file->parts[pin.part - 1];
		int pin_idx = 1;
		for (int i = m_pinSelected - 1; i >= 0; i--) {
			if (m_file->pins[i].part != pin.part)
				break;
			++pin_idx;
		}
		ImGui::Text("Part: %s   Pin: %d   Net: %s   Probe: %d   (%s.)", part.name, pin_idx, pin.net,
		            pin.probe, (part.type & 0xc) ? "SMD" : "DIP");
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
			if (m_file && ImGui::IsMouseReleased(0) && !m_draggingLastFrame) {
				ImVec2 spos = ImGui::GetMousePos();
				ImVec2 pos = ScreenToCoord(spos.x, spos.y);
				// threshold to within a pin's diameter of the pin center
				float min_dist = m_pinDiameter * 1.0f;
				min_dist *= min_dist; // all distance squared
				int selection = -1;
				for (int i = 0; i < m_file->num_pins; i++) {
					const BRDPin &pin = m_file->pins[i];
					float dx = (float)pin.pos.x - pos.x;
					float dy = (float)pin.pos.y - pos.y;
					float dist = dx * dx + dy * dy;
					if (dist < min_dist) {
						selection = i;
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

		// Rotate board: R and period rotate clockwise; comma rotates counter-clockwise
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
	}
}

void BoardView::DrawBoard() {
	if (!m_file)
		return;
	ImDrawList *draw = ImGui::GetWindowDrawList();
	if (!m_needsRedraw) {
		memcpy(draw, m_cachedDrawList, sizeof(ImDrawList));
		memcpy(draw->CmdBuffer.Data, m_cachedDrawCommands.Data, m_cachedDrawCommands.Size);
		return;
	}
	uint32_t background_color = 0xa0000000;
	// Channels:
	// 0 - images
	// 1 - polylines
	// 2 - text
	draw->ChannelsSplit(3);
	draw->ChannelsSetCurrent(1);
	ImTextureID filled_circle_tex = TextureIDs[0];
	ImTextureID empty_circle_tex = TextureIDs[1];
	for (int i = 0; i < m_file->num_format - 1; i++) {
		BRDPoint &pa = m_file->format[i];
		BRDPoint &pb = m_file->format[i + 1];
		if (pa.x == pb.x && pa.y == pb.y)
			continue;
		ImVec2 spa = CoordToScreen((float)pa.x, (float)pa.y);
		ImVec2 spb = CoordToScreen((float)pb.x, (float)pb.y);
		draw->AddLine(spa, spb, 0xff0000ff);
	}
	float psz = (float)m_pinDiameter * 0.5f * m_scale;
	char pin_number[64];
	int pin_idx = 0;
	int part_idx = 1;
	const BRDPin *selected_pin = nullptr;
	if (m_pinSelected >= 0 && m_pinSelected < m_file->num_pins) {
		selected_pin = &m_file->pins[m_pinSelected];
	}
	for (int i = 0; i < m_file->num_pins; i++) {
		const BRDPin &pin = m_file->pins[i];
		const BRDPart &part = m_file->parts[pin.part - 1];
		++pin_idx;
		if (pin.part != part_idx) {
			part_idx = pin.part;
			pin_idx = 1;
		}
		if (!PartIsVisible(part))
			continue;
		ImVec2 pos = CoordToScreen((float)pin.pos.x, (float)pin.pos.y);
		if (!IsVisibleScreen(pos.x, pos.y, psz))
			continue;
		uint32_t color = 0xffff0000;
		uint32_t text_color = color;
		bool show_text = false;
		if (m_pinHighlighted[i]) {
			text_color = color = 0xffffffff;
			show_text = true;
		}
		if (selected_pin == &pin) {
			text_color = color = 0xff00ff00;
			show_text = true;
		}
		if (PartIsHighlighted(pin.part - 1)) {
			if (!show_text) {
				color = 0xffff8000;
				text_color = 0xff808000;
			}
			show_text = true;
		}
		int pins_on_part;
		if (pin.part == 1) {
			pins_on_part = part.end_of_pins;
		} else {
			pins_on_part = part.end_of_pins - m_file->parts[pin.part - 2].end_of_pins;
		}
		if (!show_text && selected_pin && !strcmp(selected_pin->net, pin.net)) {
			color = 0xff99f8ff;
		}
		if (pins_on_part == 1)
			show_text = false;
		draw->ChannelsSetCurrent(0);
		draw->AddImage(empty_circle_tex, ImVec2(pos.x - psz, pos.y - psz),
		               ImVec2(pos.x + psz, pos.y + psz), ImVec2(0, 0), ImVec2(1, 1), color);
		if (show_text) {
			sprintf(pin_number, "%d", pin_idx);
			ImVec2 text_size = ImGui::CalcTextSize(pin_number);
			ImVec2 pos_adj = ImVec2(pos.x - text_size.x * 0.5f, pos.y - text_size.y * 0.5f);
			draw->ChannelsSetCurrent(1);
			draw->AddRectFilled(
			    ImVec2(pos_adj.x - 2.0f, pos_adj.y - 1.0f),
			    ImVec2(pos_adj.x + text_size.x + 2.0f, pos_adj.y + text_size.y + 1.0f),
			    background_color, 3.0f);
			draw->ChannelsSetCurrent(2);
			draw->AddText(pos_adj, text_color, pin_number);
		}
	}
	uint32_t box_color = 0xffcccccc;
	uint32_t part_text_color = 0xff808000;
	pin_idx = 0;
	for (int i = 0; i < m_file->num_parts; i++) {
		const BRDPart &part = m_file->parts[i];
		if (!PartIsVisible(part)) {
			pin_idx = part.end_of_pins;
			continue;
		}
		const BRDPin &pin = m_file->pins[pin_idx];
		++pin_idx;
		int pin_count = 1;
		int min_x = pin.pos.x;
		int min_y = pin.pos.y;
		int max_x = min_x;
		int max_y = min_y;
		while (pin_idx < part.end_of_pins) {
			const BRDPin &pin = m_file->pins[pin_idx];
			if (pin.pos.x > max_x)
				max_x = pin.pos.x;
			else if (pin.pos.x < min_x)
				min_x = pin.pos.x;
			if (pin.pos.y > max_y)
				max_y = pin.pos.y;
			else if (pin.pos.y < min_y)
				min_y = pin.pos.y;
			++pin_idx;
			++pin_count;
		}

		int pin_radius = m_pinDiameter / 2;
		min_x -= pin_radius;
		max_x += pin_radius;
		min_y -= pin_radius;
		max_y += pin_radius;
		bool bb_y_resized = false;
		if (part.name[0] == 'U') {
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
		uint32_t color = box_color;
		bool is_test_pad = false;
		if (!strcmp(part.name, "...")) {
			color = 0xff333333;
			is_test_pad = true;
		}
		ImVec2 min = CoordToScreen((float)min_x, (float)min_y);
		ImVec2 max = CoordToScreen((float)max_x, (float)max_y);
		min.x -= 0.5f;
		max.x += 0.5f;
		draw->AddRect(min, max, color);
		if (PartIsHighlighted(i) && !is_test_pad && part.name && part.name[0]) {
			ImVec2 text_size = ImGui::CalcTextSize(part.name);
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
			draw->ChannelsSetCurrent(1);
			draw->AddRectFilled(ImVec2(pos.x - 2.0f, pos.y - 1.0f),
			                    ImVec2(pos.x + text_size.x + 2.0f, pos.y + text_size.y + 1.0f),
			                    background_color, 3.0f);
			draw->ChannelsSetCurrent(2);
			draw->AddText(pos, part_text_color, part.name);
		}
	}
	draw->ChannelsMerge();

	// Copy the new draw list and cmd buffer:
	memcpy(m_cachedDrawList, draw, sizeof(ImDrawList));
	int cmds_size = draw->CmdBuffer.size() * sizeof(ImDrawCmd);
	m_cachedDrawCommands.resize(cmds_size);
	memcpy(m_cachedDrawCommands.Data, draw->CmdBuffer.Data, cmds_size);
	m_needsRedraw = false;
}

int qsort_netstrings(const void *a, const void *b) {
	const char *sa = *(const char **)a;
	const char *sb = *(const char **)b;
	return strcmp(sa, sb);
}

void BoardView::SetFile(BRDFile *file) {
	delete m_file;
	m_file = file;

	// Generate sorted, uniqued list of nets:
	m_nets.resize(0);
	m_nets.reserve(m_file->num_pins);
	for (int i = 0; i < m_file->num_pins; i++) {
		m_nets.push_back(m_file->pins[i].net);
	}
	qsort(m_nets.Data, m_nets.Size, sizeof(char *), qsort_netstrings);
	{
		int i = 0, j = 0;
		while (++i < m_nets.Size) {
			if (strcmp(m_nets[i], m_nets[j])) {
				m_nets[++j] = m_nets[i];
			}
		}
		m_nets.Size = j;
	}

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
	float dx = 1.05f * (max_x - min_x);
	float dy = 1.05f * (max_y - min_y);
	ImVec2 view = ImGui::GetIO().DisplaySize;
	float sx = dx > 0 ? view.x / dx : 1.0f;
	float sy = dy > 0 ? view.y / dy : 1.0f;
	m_scale = sx < sy ? sx : sy;
	m_boardWidth = max_x - min_x;
	m_boardHeight = max_y - min_y;
	SetTarget(m_mx, m_my);

	m_pinHighlighted.Resize(m_file->num_pins);
	m_partHighlighted.Resize(m_file->num_parts);
	m_pinSelected = -1;

	m_firstFrame = true;
	m_needsRedraw = true;
}

ImVec2 BoardView::CoordToScreen(float x, float y, float w) {
	float side = m_side ? -1.0f : 1.0f;
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
	float side = m_side ? -1.0f : 1.0f;
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
		if (m_side == 0) {
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
		if (m_side == 1) {
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

bool BoardView::PartIsVisible(const BRDPart &part) {
	if (m_side == 1) {
		if (part.type < 8 && part.type >= 4)
			return false;
	} else {
		if (part.type >= 8)
			return false;
	}
	return true;
}

bool BoardView::IsVisibleScreen(float x, float y, float radius) {
	const ImGuiIO &io = ImGui::GetIO();
	if (x < -radius || y < -radius || x - radius > io.DisplaySize.x ||
	    y - radius > io.DisplaySize.y)
		return false;
	return true;
}

bool BoardView::PartIsHighlighted(int part_idx) {
	bool highlighted = m_partHighlighted[part_idx];
	if (m_pinSelected >= 0 && m_pinSelected < m_file->num_pins) {
		highlighted |= part_idx == (m_file->pins[m_pinSelected].part - 1);
	}
	return highlighted;
}

void BoardView::SetNetFilter(const char *net) {
	strcpy(m_netFilter, net);
	if (!m_file)
		return;

	m_pinHighlighted.Clear();
	m_partHighlighted.Clear();
	if (m_netFilter[0]) {
		bool any_visible = false;
		int count = 0;
		for (int i = 0; i < m_file->num_pins; i++) {
			const BRDPin &pin = m_file->pins[i];
			if (!stricmp(m_netFilter, pin.net)) {
				any_visible |= PartIsVisible(m_file->parts[pin.part - 1]);
				m_pinHighlighted.Set(i, true);
				m_partHighlighted.Set(pin.part - 1, true);
				count++;
			}
		}
		if (count > 0) {
			if (!any_visible)
				FlipBoard();
			m_pinSelected = -1;
		}
	}
	m_needsRedraw = true;
}

void BoardView::FindComponent(const char *name) {
	if (!m_file)
		return;

	m_pinHighlighted.Clear();
	m_partHighlighted.Clear();
	if (name[0]) {
		int part_idx = -1;
		bool any_visible = false;
		for (int i = 0; i < m_file->num_parts; i++) {
			const BRDPart &part = m_file->parts[i];
			if (!stricmp(name, part.name)) {
				part_idx = i;
				m_partHighlighted.Set(part_idx, true);
				any_visible |= PartIsVisible(part);
			}
		}
		if (part_idx >= 0) {
			if (!any_visible)
				FlipBoard();
			m_pinSelected = -1;
		}
		for (int i = 0; i < m_file->num_pins; i++) {
			const BRDPin &pin = m_file->pins[i];
			if (pin.part - 1 == part_idx) {
				m_pinHighlighted.Set(i, true);
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
	m_side ^= 1;
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
