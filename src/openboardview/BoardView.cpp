#include "platform.h" // Should be kept first
#include "BoardView.h"
#include "history.h"
#include "utf8/utf8.h"
#include "utils.h"

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
#include "FileFormats/BRDAllegroFile.h"
#include "FileFormats/BRDFile.h"
#include "FileFormats/BVRFile.h"
#include "FileFormats/BVR3File.h"
#include "FileFormats/CADFile.h"
#include "FileFormats/CSTFile.h"
#include "FileFormats/FZFile.h"
#include "FileFormats/GenCADFile.h"
#include "FileFormats/XZZPCBFile.h"
#include "GUI/DPI.h"
#include "GUI/widgets.h"
#include "annotations.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h" // For ImGui::FocusWindow()
#include "imgui/misc/cpp/imgui_stdlib.h"

#include "NetList.h"
#include "PartList.h"
#include "vectorhulls.h"

#if _MSC_VER
#define stricmp _stricmp
#endif

#ifndef _WIN32
#define stricmp strcasecmp
#endif

BoardView::~BoardView() {
	if (m_validBoard) {
		m_board->Nets().clear();
		m_board->Pins().clear();
		m_board->Components().clear();
		m_board->OutlinePoints().clear();
		m_board->OutlineSegments().clear();
		delete m_file;
		delete m_board;
		m_annotations.Close();
		m_validBoard = false;
	}
}

int BoardView::ConfigParse(void) {
	ImGuiStyle &style = ImGui::GetStyle();

	config.readFromConfig(obvconfig);

	setDPI(config.dpi);

	style.AntiAliasedLines = !config.slowCPU;
	style.AntiAliasedFill  = !config.slowCPU;

	m_info_surface.x = config.infoPanelWidth;
	backgroundImage.enabled = config.showBackgroundImage;

	m_colors.readFromConfig(obvconfig);
	keybindings.readFromConfig(obvconfig);

	return 0;
}

int BoardView::LoadFile(const filesystem::path &filepath) {
	m_lastFileOpenWasInvalid = true;
	m_validBoard             = false;
	if (!filepath.empty()) {
		// clean up the previous file.
		if (m_file && m_board) {
			m_pinHighlighted.clear();
			m_partHighlighted.clear();
			m_annotations.Close();
			m_board->Nets().clear();
			m_board->Pins().clear();
			m_board->Components().clear();
			m_board->OutlinePoints().clear();
			m_board->OutlineSegments().clear();
			//	delete m_file;
			// delete m_board;
		}
		delete m_file;
		m_file = nullptr;
		m_validBoard = false;
		m_error_msg.clear();
		pdfBridge.CloseDocument();

		SetLastFileOpenName(filepath.string());
		std::vector<char> buffer = file_as_buffer(filepath, m_error_msg);
		if (!buffer.empty()) {
			if (check_fileext(filepath, ".fz")) { // Since it is encrypted we cannot use the below logic. Trust the ext.
				m_file = new FZFile(buffer, config.FZKey);
			} else if (check_fileext(filepath, ".bom") || check_fileext(filepath, ".asc"))
				m_file = new ASCFile(buffer, filepath);
			else if (GenCADFile::verifyFormat(buffer))
				m_file = new GenCADFile(buffer);
			else if (ADFile::verifyFormat(buffer))
				m_file = new ADFile(buffer);
			else if (CADFile::verifyFormat(buffer))
				m_file = new CADFile(buffer);
			else if (check_fileext(filepath, ".cst"))
				m_file = new CSTFile(buffer);
			else if (BRDFile::verifyFormat(buffer))
				m_file = new BRDFile(buffer);
			else if (BRD2File::verifyFormat(buffer))
				m_file = new BRD2File(buffer);
			else if (BDVFile::verifyFormat(buffer))
				m_file = new BDVFile(buffer);
			else if (BVRFile::verifyFormat(buffer))
				m_file = new BVRFile(buffer);
			else if (BVR3File::verifyFormat(buffer))
				m_file = new BVR3File(buffer);
			else if (BRDAllegroFile::verifyFormat(buffer))
				m_file = new BRDAllegroFile(buffer);
			else if (XZZPCBFile::verifyFormat(buffer))
				m_file = new XZZPCBFile(buffer, filepath.string());
			else
				m_error_msg = "Unrecognized file format.";

			if (m_file && m_file->valid) {
				LoadBoard(m_file);
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
				pdfFile.loadFromConfig(conffilepath);

				pdfBridge.OpenDocument(pdfFile);

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
				m_error_msg.clear();
			}
		}
	} else {
		return 1;
	}

	return 0;
}


void BoardView::ShowInfoPane(void) {
	ImGuiIO &io = ImGui::GetIO();
	ImVec2 ds   = io.DisplaySize;

	if (!config.showInfoPanel) return;

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
			config.infoPanelWidth = m_info_surface.x;
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
		if (ImGui::Checkbox("Zoom on selected net", &config.infoPanelCenterZoomNets)) {
			obvconfig.WriteBool("infoPanelCenterZoomNets", config.infoPanelCenterZoomNets);
		}
		if (ImGui::Checkbox("Select all parts on net", &config.infoPanelSelectPartsOnNet)) {
			obvconfig.WriteBool("infoPanelSelectPartsOnNet", config.infoPanelSelectPartsOnNet);
		}
	} else {
		ImGui::Text("No board currently loaded.");
	}

	if (m_partHighlighted.size()) {
		ImGui::Separator();
		ImGui::TextUnformatted((std::to_string(m_partHighlighted.size()) + " parts selected").c_str());

		for (auto part : m_partHighlighted) {

			ImGui::Text(" ");

			if (ImGui::SmallButton(part->name.c_str())) {
				if (!BoardElementIsVisible(part)) FlipBoard();
				if (part->centerpoint.x && part->centerpoint.y) {
					ImVec2 psz;

					/*
					 * Check to see if we need to zoom BACK a bit to fit the part in to view
					 */
					psz = ImVec2(abs(part->outline[2].x - part->outline[0].x) * m_scale / m_board_surface.x,
					             abs(part->outline[2].y - part->outline[0].y) * m_scale / m_board_surface.y);
					if ((psz.x > 1) || (psz.y > 1)) {
						if (psz.x > psz.y) {
							m_scale /= (config.partZoomScaleOutFactor * psz.x);
						} else {
							m_scale /= (config.partZoomScaleOutFactor * psz.y);
						}
					}

					SetTarget(part->centerpoint.x, part->centerpoint.y);
				} else {
					SetTarget(part->pins[0]->position.x, part->pins[0]->position.y);
				}
				m_needsRedraw = 1;
			}
			ImGui::SameLine();
			{
				char bn[128];
				snprintf(bn, sizeof(bn), "Z##%s", part->name.c_str());
				if (ImGui::SmallButton(bn)) {
					if (!BoardElementIsVisible(part)) FlipBoard();
					if (part->centerpoint.x && part->centerpoint.y) {
						ImVec2 psz;

						/*
						 * Check to see if we need to zoom BACK a bit to fit the part in to view
						 */
						psz = ImVec2(abs(part->outline[2].x - part->outline[0].x) * m_scale / m_board_surface.x,
						             abs(part->outline[2].y - part->outline[0].y) * m_scale / m_board_surface.y);
						if (psz.x > psz.y) {
							m_scale /= (config.partZoomScaleOutFactor * psz.x);
						} else {
							m_scale /= (config.partZoomScaleOutFactor * psz.y);
						}

						SetTarget(part->centerpoint.x, part->centerpoint.y);
					} else {
						SetTarget(part->pins[0]->position.x, part->pins[0]->position.y);
					}
					m_needsRedraw = 1;
				}
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

			{
				static bool wholeWordsOnly = true;
				static bool caseSensitive = false;
				std::string pdfButtonName = "PDF Search##" + part->name;
				if (ImGui::SmallButton(pdfButtonName.c_str())) {
					pdfBridge.DocumentSearch(part->name, wholeWordsOnly, caseSensitive);
				}
				ImGui::SameLine();
				ImGui::Checkbox("Whole words only", &wholeWordsOnly);
				ImGui::SameLine();
				ImGui::Checkbox("Case sensitive", &caseSensitive);
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
			if (ImGui::BeginListBox(str.c_str(), listSize)) { //, ImVec2(m_board_surface.x/3 -5, m_board_surface.y/2));
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
							for (auto p : m_partHighlighted) {
								pin->component->visualmode = pin->component->CVMNormal;
							};
							m_partHighlighted.push_back(pin->component);
							CenterZoomNet(pin->net->name);
						}
						m_needsRedraw = true;
					}
					ImGui::PushStyleColor(ImGuiCol_Border, 0xffeeeeee);
					ImGui::Separator();
					ImGui::PopStyleColor();
				}
				ImGui::EndListBox();
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
						auto poly = part->outline;

						for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
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
							ImGui::CloseCurrentPopup();
						}
						ImGui::SameLine();
						if (ImGui::Button("Cancel##1")) {
							ImGui::CloseCurrentPopup();
							m_annotationnew_retain = false;
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
					ImGui::CloseCurrentPopup();
				}
			}

			// the position.
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel##2") || keybindings.isPressed("CloseDialog")) {
			m_annotationnew_retain  = false;
			m_annotationedit_retain = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	ImGui::PopStyleVar();

	/** if the dialog was closed by using the (X) icon **/
	if (!dummy) {
		m_annotationnew_retain  = false;
		m_annotationedit_retain = false;
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
	if (ImGui::BeginListBox(title.c_str())) {

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

		ImGui::EndListBox();
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

		bool search_params_changed = m_showSearch; //treat initiail popup reopening similar to later search option changes
		if (m_showSearch) {
			m_showSearch       = false;
			//			fprintf(stderr, "Tooltips disabled\n");
		}

		// Column 1, implied.
		//
		//
		if (ImGui::Button("Search")) {
			// FindComponent(first_button);
			SearchCompound(first_button[0]);
			SearchCompoundNoClear(first_button[1]);
			SearchCompoundNoClear(first_button[2]);
			CenterZoomSearchResults();
			ImGui::CloseCurrentPopup();
		} // search button

		ImGui::SameLine();
		if (ImGui::Button("Reset")) {
			FindComponent("");
			ResetSearch();
		} // reset button

		ImGui::SameLine();
		if (ImGui::Button("Exit") || keybindings.isPressed("CloseDialog")) {
			FindComponent("");
			ResetSearch();
			ImGui::CloseCurrentPopup();
		} // exit button

		ImGui::SameLine();
		//		ImGui::Dummy(ImVec2(DPI(200), 1));
		//		ImGui::SameLine();
		ImGui::PushItemWidth(-1);
		ImGui::Text("ENTER: Search, ESC: Exit, TAB: next field");

		{
			ImGui::Separator();
			search_params_changed |= ImGui::Checkbox("Components", &m_searchComponents);

			ImGui::SameLine();
			search_params_changed |= ImGui::Checkbox("Nets", &m_searchNets);

			ImGui::SameLine();
			search_params_changed |= ImGui::Checkbox("Including details", &searcher.configSearchDetails());

			ImGui::Text(" Search mode: ");
			ImGui::SameLine();
			if (ImGui::RadioButton("Substring", searcher.isMode(SearchMode::Sub))) {
				search_params_changed = true;
				searcher.setMode(SearchMode::Sub);
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("Prefix", searcher.isMode(SearchMode::Prefix))) {
				search_params_changed = true;
				searcher.setMode(SearchMode::Prefix);
			}
			ImGui::SameLine();
			ImGui::PushItemWidth(-1);
			if (ImGui::RadioButton("Whole", searcher.isMode(SearchMode::Whole))) {
				search_params_changed = true;
				searcher.setMode(SearchMode::Whole);
			}
			ImGui::PopItemWidth();
		}

		ImGui::Separator();

		ImGui::Columns(3);

		for (int i = 0; i < 3; i++) {
			std::string ui_number   = std::to_string(i + 1); // visual UI number is 1-based unlike 0-based indexing
			std::string title       = "Item #" + ui_number;
			std::string searchLabel = "##search" + ui_number;
			ImGui::Text("%s", title.c_str());

			ImGui::PushItemWidth(-1);

			bool textNonEmpty = m_search[i][0] != '\0';                            // Text typed in the search box
			auto results      = SearchPartsAndNets(m_search[i], 30);               // Perform the search for both nets and parts
			bool hasResults   = !results.first.empty() || !results.second.empty(); // We found some nets or some parts

			if (textNonEmpty && !hasResults) ImGui::PushStyleColor(ImGuiCol_FrameBg, 0xFF6666FF);
			bool textChanged =
			    ImGui::InputText(searchLabel.c_str(),
			                     m_search[i],
			                     128,
			                     ImGuiInputTextFlags_CharsNoBlank | (m_search[0] ? ImGuiInputTextFlags_AutoSelectAll : 0));
			if (textNonEmpty && !hasResults) ImGui::PopStyleColor();
			if (ImGui::IsItemActivated()) {
				// user activates another column
				m_active_search_column = i;
				search_params_changed  = true;
			}

			bool this_column_active = i == m_active_search_column;

			if (textChanged || (search_params_changed && this_column_active)) SearchCompound(m_search[i]);

			ImGui::PopItemWidth();

			if (!ImGui::IsItemDeactivated() && this_column_active &&
			    ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive() &&
			    !ImGui::IsMouseClicked(0)) {
				ImGui::SetKeyboardFocusHere(-1);
			} // set keyboard focus back to active colun input-text after other type of element was clicked

			ImGui::PushItemWidth(-1);
			if (textNonEmpty) SearchColumnGenerate("##SC" + ui_number, results, m_search[i], 30);
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
		} // response to keyboard ENTER

		ImGui::EndPopup();
	}
	ImGui::PopStyleVar();
}

void BoardView::ResetSearch() {
	for (int i = 0; i < 3; i++) m_search[i][0] = '\0';
	m_active_search_column = 0;
}

void BoardView::ClearAllHighlights(void) {
	m_pinSelected = nullptr;
	FindNet("");
	FindComponent("");
	ResetSearch();
	m_needsRedraw      = true;
	if (m_board != NULL) {
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
	char *preset_filename = NULL;
	ImGuiIO &io           = ImGui::GetIO();

	// Window is probably minimized, do not attempt to draw anything as our code will not handle screen size of 0 properly and crash (ocornut/imgui@bb2529d)
	if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f) {
		return;
	}

	/**
	 * ** FIXME
	 * This should be handled in the keyboard section, not here
	 */
	if (keybindings.isPressed("Open")) {
		open_file = true;
		// the dialog will likely eat our WM_KEYUP message for CTRL and O:
		io.AddKeyEvent(ImGuiKey_RightCtrl, false);
		io.AddKeyEvent(ImGuiKey_LeftCtrl, false);
		io.AddKeyEvent(ImGuiKey_O, false);

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

			programPreferences.menuItem();
			colorPreferences.menuItem();
			keyboardPreferences.menuItem();
			boardSettings.menuItem();

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

			ImGui::Separator();
			if (MenuItemWithCheckbox("Show FPS", {}, config.showFPS)) {
				obvconfig.WriteBool("showFPS", config.showFPS);
			}

			if (MenuItemWithCheckbox("Show position", {}, config.showPosition)) {
				obvconfig.WriteBool("showPosition", config.showPosition);
			}

			if (MenuItemWithCheckbox("Show pins", keybindings.getKeyNames("TogglePins"), config.showPins)) {
				obvconfig.WriteBool("showPins", config.showPins);
				m_needsRedraw = true;
			}

			if (MenuItemWithCheckbox("Show net web", {}, config.showNetWeb)) {
				obvconfig.WriteBool("showNetWeb", config.showNetWeb);
				m_needsRedraw = true;
			}

			if (MenuItemWithCheckbox("Show annotations", {}, config.showAnnotations)) {
				obvconfig.WriteBool("showAnnotations", config.showAnnotations);
				m_needsRedraw = true;
			}


			if (MenuItemWithCheckbox("Show parts name", {}, config.showPartName)) {
				obvconfig.WriteBool("showPartName", config.showPartName);
				m_needsRedraw = true;
			}

			if (MenuItemWithCheckbox("Show pins name", {}, config.showPinName)) {
				obvconfig.WriteBool("showPinName", config.showPinName);
				m_needsRedraw = true;
			}

			if (MenuItemWithCheckbox("Show background image", {}, config.showBackgroundImage)) {
				obvconfig.WriteBool("showBackgroundImage", config.showBackgroundImage);
				backgroundImage.enabled = config.showBackgroundImage;
			}

			if (MenuItemWithCheckbox("Fill board", {}, config.boardFill)) {
				obvconfig.WriteBool("boardFill", config.boardFill);
				m_needsRedraw = true;
			}

			if (MenuItemWithCheckbox("Fill parts", {}, config.fillParts)) {
				obvconfig.WriteBool("fillParts", config.fillParts);
				m_needsRedraw = true;
			}

			ImGui::Separator();

			if (MenuItemWithCheckbox("Info Panel", keybindings.getKeyNames("InfoPanel"), config.showInfoPanel)) {
				obvconfig.WriteBool("showInfoPanel", config.showInfoPanel);
			}
			MenuItemWithCheckbox("Net List", keybindings.getKeyNames("NetList"), m_showNetList);
			MenuItemWithCheckbox("Part List", keybindings.getKeyNames("PartList"), m_showPartList);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help##1")) {
			helpControls.menuItem();
			helpAbout.menuItem();

			ImGui::EndMenu();
		}

		ImGui::SameLine();
		ImGui::Dummy(ImVec2(DPI(10), 1));
		ImGui::SameLine();
		if (ImGui::Checkbox("Annotations", &config.showAnnotations)) {
			obvconfig.WriteBool("showAnnotations", config.showAnnotations);
			m_needsRedraw = true;
		}

		ImGui::SameLine();
		if (ImGui::Checkbox("Netweb", &config.showNetWeb)) {
			obvconfig.WriteBool("showNetWeb", config.showNetWeb);
			m_needsRedraw = true;
		}

		ImGui::SameLine();
		{
			if (ImGui::Checkbox("Pins", &config.showPins)) {
				obvconfig.WriteBool("showPins", config.showPins);
				m_needsRedraw = true;
			}
		}

		ImGui::SameLine();
		if (ImGui::Checkbox("Image", &config.showBackgroundImage)) {
			obvconfig.WriteBool("showBackgroundImage", config.showBackgroundImage);
			backgroundImage.enabled = config.showBackgroundImage;
			m_needsRedraw = true;
		}

		ImGui::SameLine();
		ImGui::Dummy(ImVec2(DPI(40), 1));

		ImGui::SameLine();
		if (ImGui::Button(" - ")) {
			Zoom(m_board_surface.x / 2, m_board_surface.y / 2, -config.zoomFactor);
		}
		ImGui::SameLine();
		if (ImGui::Button(" + ")) {
			Zoom(m_board_surface.x / 2, m_board_surface.y / 2, config.zoomFactor);
		}
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(DPI(20), 1));
		ImGui::SameLine();
		if (ImGui::Button("-")) {
			Zoom(m_board_surface.x / 2, m_board_surface.y / 2, -config.zoomFactor / config.zoomModifier);
		}
		ImGui::SameLine();
		if (ImGui::Button("+")) {
			Zoom(m_board_surface.x / 2, m_board_surface.y / 2, config.zoomFactor / config.zoomModifier);
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

		if (m_showContextMenu && m_file && config.showAnnotations) {
			ImGui::OpenPopup("Annotations");
		}

		helpAbout.render();
		helpControls.render();

		programPreferences.render();

		if (colorPreferences.render()) {
			m_needsRedraw = true;
		};

		keyboardPreferences.render();
		boardSettings.render();

		if (m_showSearch && m_file) {
			ImGui::OpenPopup("Search for Component / Network");
		}

		if (ImGui::BeginPopupModal("Error opening file")) {
			ImGui::Text("There was an error opening the file: %s", m_lastFileOpenName.c_str());
			if (!m_error_msg.empty()) {
				ImGui::Text("%s", m_error_msg.c_str());
			}
			if (m_file && !m_file->error_msg.empty()) {
				ImGui::Text("%s", m_file->error_msg.c_str());
			}
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

	// ImGui's keyboard navigation is disabled for the boardview area as we use our own key bindings
	ImGuiWindowFlags draw_surface_flags = flags | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs;

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
		ImVec2 pos  = ScreenToCoord(spos.x, spos.y);
		if (config.showFPS == true) {
			ImGui::Text("FPS: %0.0f ", ImGui::GetIO().Framerate);
			ImGui::SameLine();
		}

		if (debug) {
			ImGui::Text("AnnID:%d ", m_annotation_clicked_id);
			ImGui::SameLine();
		}

		if (config.showPosition == true) {
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

	if (!config.showInfoPanel) {
		m_board_surface = ImVec2(io.DisplaySize.x, io.DisplaySize.y - (m_status_height + m_menu_height));
	} else {
		m_board_surface = ImVec2(io.DisplaySize.x - m_info_surface.x, io.DisplaySize.y - (m_status_height + m_menu_height));
	}
	/*
	 * Drawing surface, where the actual PCB/board is plotted out
	 */
	ImGui::SetNextWindowPos(ImVec2(0, m_menu_height));
	if (io.DisplaySize.x != m_lastWidth || io.DisplaySize.y != m_lastHeight) {
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
	if (m_validBoard) {
		HandleInput();
		backgroundImage.render(*ImGui::GetWindowDrawList(),
			CoordToScreen(backgroundImage.x0(), backgroundImage.y0()),
			CoordToScreen(backgroundImage.x1(), backgroundImage.y1()),
			m_rotation);
		DrawBoard();
	}
	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();

	// Overlay
	RenderOverlay();

	ImGui::PopStyleVar();

	HandlePDFBridgeSelection();

} // main menu bar

void BoardView::Zoom(float osd_x, float osd_y, float zoom) {
	ImVec2 target;
	ImVec2 coord;
	ImGuiIO &io = ImGui::GetIO();

	if (io.KeyCtrl) zoom /= config.zoomModifier;

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

	if (io.KeyCtrl) amount /= config.panModifier;

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
	if (!m_board || (!m_file)) return;

	const ImGuiIO &io = ImGui::GetIO();

	if (ImGui::IsWindowHovered()) {
		if (!ImGui::IsWindowFocused()) {
			// Set focus on boardview area hover to apply our key bindings instead of the currently active ImGui's keyboard navigation
			// Using imgui_internal's FocusWindow with UnlessBelowModal instead of SetWindowFocus
			// Otherwise PopupModal get closed right after opening when boardview area is hovered
			// https://github.com/ocornut/imgui/issues/3595
			// Warning: wouldn't work with regular Popup but we use only PopupModal
			ImGui::FocusWindow(nullptr, ImGuiFocusRequestFlags_UnlessBelowModal);
		}

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
				ImVec2 td = ScreenToCoord(delta.x, delta.y, 0);
				m_dx += td.x;
				m_dy += td.y;
				m_draggingLastFrame = true;
				m_needsRedraw       = true;
			}
		} else if (m_dragging_token >= 0) {
			m_dragging_token = 0;

			if (m_lastFileOpenWasInvalid == false) {
				// Conext menu
				if (!m_lastFileOpenWasInvalid && m_file && m_board && ImGui::IsMouseClicked(1)) {
					if (config.showAnnotations) {
						// Build context menu here, for annotations and inspection
						//
						ImVec2 spos = ImGui::GetMousePos();
						if (AnnotationIsHovered()) m_annotation_clicked_id = m_annotation_last_hovered;

						m_annotationedit_retain = false;
						m_showContextMenu       = true;
						m_showContextMenuPos    = spos;
						m_needsRedraw           = true;
						if (debug) fprintf(stderr, "context click request at (%f %f)\n", spos.x, spos.y);
					}

					// Flip the board with the middle click
				} else if (!m_lastFileOpenWasInvalid && m_file && m_board && ImGui::IsMouseReleased(2)) {
					FlipBoard();

					// Else, click to select pin
				} else if (!m_lastFileOpenWasInvalid && m_file && m_board && ImGui::IsMouseReleased(0) && !m_draggingLastFrame) {
					ImVec2 spos = ImGui::GetMousePos();
					ImVec2 pos  = ScreenToCoord(spos.x, spos.y);

					m_needsRedraw = true;

					// threshold to within a pin's diameter of the pin center
					// float min_dist = m_pinDiameter * 1.0f;
					float min_dist = m_pinDiameter / 2.0f;
					min_dist *= min_dist; // all distance squared
					std::shared_ptr<Pin> selection = nullptr;
					for (auto &pin : m_board->Pins()) {
						if (BoardElementIsVisible(pin)) {
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
								auto poly = part->outline;

								for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
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
				mwheel *= config.zoomFactor;

				Zoom(io.MousePos.x, io.MousePos.y, mwheel);
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
			Zoom(m_board_surface.x / 2, m_board_surface.y / 2, config.zoomFactor);

		} else if (keybindings.isPressed("ZoomOut")) {
			Zoom(m_board_surface.x / 2, m_board_surface.y / 2, -config.zoomFactor);

		} else if (keybindings.isPressed("PanDown")) {
			Pan(DIR_DOWN, DPI(config.panFactor));

		} else if (keybindings.isPressed("PanUp")) {
			Pan(DIR_UP, DPI(config.panFactor));

		} else if (keybindings.isPressed("PanLeft")) {
			Pan(DIR_LEFT, DPI(config.panFactor));

		} else if (keybindings.isPressed("PanRight")) {
			Pan(DIR_RIGHT, DPI(config.panFactor));

		} else if (keybindings.isPressed("Center")) {
			// Center and reset zoom
			CenterView();

		} else if (keybindings.isPressed("Quit")) {
			// quit OFBV
			m_wantsQuit = true;

		} else if (keybindings.isPressed("InfoPanel")) {
			config.showInfoPanel = !config.showInfoPanel;
			obvconfig.WriteBool("showInfoPanel", config.showInfoPanel ? true : false);

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
			config.showPins ^= 1;
			m_needsRedraw = true;

		} else if (keybindings.isPressed("Search")) {
			if (m_validBoard) {
				m_showSearch  = true;
				m_needsRedraw = true;
			}

		} else if (keybindings.isPressed("Clear")) {
			ClearAllHighlights();

		} else {
			/*
			 * Do what ever is required for unhandled key presses.
			 */
		}
	}
}

/* END UPDATE REGION */

/** Overlay and Windows
 *
 *
 */
void BoardView::ShowNetList(bool *p_open) {
	static NetList netList(keybindings, std::bind(&BoardView::FindNet, this, std::placeholders::_1));
	netList.Draw("Net List", p_open, m_board);
}

void BoardView::ShowPartList(bool *p_open) {
	static PartList partList(keybindings, std::bind(&BoardView::FindComponent, this, std::placeholders::_1));
	partList.Draw("Part List", p_open, m_board);
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

void BoardView::CenterZoomNet(std::string netname) {
	ImVec2 view = m_board_surface;
	ImVec2 min, max;
	int i = 0;

	if (!config.infoPanelCenterZoomNets) return;

	min.x = min.y = FLT_MAX;
	max.x = max.y = FLT_MIN;

	for (auto &pin : m_board->Pins()) {
		if (pin->net->name == netname) {
			auto p = pin->position;
			if (p.x < min.x) min.x = p.x;
			if (p.y < min.y) min.y = p.y;
			if (p.x > max.x) max.x = p.x;
			if (p.y > max.y) max.y = p.y;

			if ((config.infoPanelSelectPartsOnNet) && (pin->type != Pin::kPinTypeTestPad)) {
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
	m_scale /= config.partZoomScaleOutFactor;
	if (m_scale < m_scale_floor) m_scale = m_scale_floor;

	/*
	float dx = (max.x - min.x);
	float dy = (max.y - min.y);
	float sx = dx > 0 ? view.x / dx : 1.0f;
	float sy = dy > 0 ? view.y / dy : 1.0f;

	//  m_rotation = 0;
	m_scale = sx < sy ? sx : sy;
	m_scale /= config.partZoomScaleOutFactor;
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

	if (!config.showPins) config.showPins = true; // Louis Rossmann UI failure fix.

	if (!config.centerZoomSearchResults) return;

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
	m_scale /= config.partZoomScaleOutFactor;
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

	if (outline.empty()) {
		return 1;
	};

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
	std::vector<ImVec2> scanhits;
	static ImVec2 min, max; // board min/max points
	                        //	double steps = 500;
	double vdelta;
	double y, ystart, yend;

	if (!config.boardFill || config.slowCPU) return;
	if (!m_file) return;

	scanhits.reserve(20);

	draw->ChannelsSetCurrent(kChannelFill);

	// find the orthagonal bounding box
	// probably can put this as a predefined
	if (!boardMinMaxDone) {
		min.x = min.y = 100000000000;
		max.x = max.y = -100000000000;
		for (auto &p : m_board->OutlinePoints()) {
			if (p->x < min.x) min.x = p->x;
			if (p->y < min.y) min.y = p->y;
			if (p->x > max.x) max.x = p->x;
			if (p->y > max.y) max.y = p->y;
		}
		for (auto &s: m_board->OutlineSegments()) {
			if (s.first.x < min.x) min.x = s.first.x;
			if (s.second.x < min.x) min.x = s.second.x;
			if (s.first.y < min.y) min.y = s.first.y;
			if (s.second.y < min.y) min.y = s.second.y;
			if (s.first.x > max.x) max.x = s.first.x;
			if (s.second.x > max.x) max.x = s.second.x;
			if (s.first.y > max.y) max.y = s.first.y;
			if (s.second.y > max.y) max.y = s.second.y;
		}
		boardMinMaxDone = true;
	}

	// Get the viewport limits, so we don't waste time scanning what we don't need
	ImVec2 vpa = ScreenToCoord(0, 0);
	ImVec2 vpb = ScreenToCoord(io.DisplaySize.x, io.DisplaySize.y);

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

			auto &outline_points = m_board->OutlinePoints();

			// set our initial draw point, so we can detect when we encounter it again
			if (!outline_points.empty()) {
				fp = *outline_points[0];

				for (size_t i = 0; i < outline_points.size() - 1; i++) {
					Point &pa = *outline_points[i];
					Point &pb = *outline_points[i + 1];

					// jump double/dud points
					if (pa.x == pb.x && pa.y == pb.y) continue;

					// if we encounter our hull/poly start point, then we've now created the
					// closed
					// hull, jump the next segment and reset the first-point
					if ((!jump) && (fp.x == pb.x) && (fp.y == pb.y)) {
						if (i < outline_points.size() - 2) {
							fp   = *outline_points[i + 2];
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
			}

			for (auto &s: m_board->OutlineSegments()) {
				auto pa = s.first;
				auto pb = s.second;

				// jump double/dud segments
				if (pa.x == pb.x && pa.y == pb.y) continue;

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
			}

			sort(scanhits.begin(), scanhits.end(), [](ImVec2 const &a, ImVec2 const &b) { return a.x < b.x; });
			// Some boards contain duplicate outline segments (possibly with points swapped) that generate duplicate intersections
			// which interferes with the process of generating alterating segments for the scanlines, so remove duplicates.
			scanhits.erase(std::unique(scanhits.begin(), scanhits.end(), [](const ImVec2 &a, const ImVec2 &b) {return a.x == b.x && a.y == b.y;}), scanhits.end());

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

void BoardView::DrawOutlineSegments(ImDrawList *draw) {
	const auto &segments = m_board->OutlineSegments();

	draw->ChannelsSetCurrent(kChannelPolylines);

	for (auto &segment: segments) {
		ImVec2 spa = CoordToScreen(segment.first.x, segment.first.y);
		ImVec2 spb = CoordToScreen(segment.second.x, segment.second.y);

		/*
		 * If we have a pin selected, we mask off the colour to shade out
		 * things and make it easier to see associated pins/points
		 */
		if ((config.pinSelectMasks) && (m_pinSelected || m_pinHighlighted.size())) {
			draw->AddLine(spa, spb, (m_colors.boardOutlineColor & m_colors.selectedMaskOutline) | m_colors.orMaskOutline);
		} else {
			draw->AddLine(spa, spb, m_colors.boardOutlineColor);
		}
	}
}

void BoardView::DrawOutlinePoints(ImDrawList *draw) {
	int jump = 1;
	Point fp;

	auto &outline = m_board->OutlinePoints();
	if (outline.size() < 1) { // Nothing to draw
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
		if ((config.pinSelectMasks) && (m_pinSelected || m_pinHighlighted.size())) {
			draw->AddLine(spa, spb, (m_colors.boardOutlineColor & m_colors.selectedMaskOutline) | m_colors.orMaskOutline);
		} else {
			draw->AddLine(spa, spb, m_colors.boardOutlineColor);
		}
	} // for
}

void BoardView::DrawOutline(ImDrawList *draw) {
	DrawOutlineSegments(draw);
	DrawOutlinePoints(draw);
}

void BoardView::DrawNetWeb(ImDrawList *draw) {
	if (!config.showNetWeb) return;

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
				draw->AddCircle(CoordToScreen(p->position.x, p->position.y), p->diameter * m_scale, col, 16);
			}

			draw->AddLine(CoordToScreen(m_pinSelected->position.x, m_pinSelected->position.y),
			              CoordToScreen(p->position.x, p->position.y),
			              ImColor(col),
			              config.netWebThickness);
		}
	}

	return;
}

inline void BoardView::DrawPins(ImDrawList *draw) {

	uint32_t cmask  = 0xFFFFFFFF;
	uint32_t omask  = 0x00000000;
	float threshold = 0;
	auto io         = ImGui::GetIO();

	if (!config.showPins) return;

	/*
	 * If we have a pin selected, then it makes it
	 * easier to see where the associated pins are
	 * by masking out (alpha or channel) the other
	 * pins so they're fainter.
	 */
	if (config.pinSelectMasks) {
		if (m_pinSelected || m_pinHighlighted.size()) {
			cmask = m_colors.selectedMaskPins;
			omask = m_colors.orMaskPins;
		}
	}

	if (config.slowCPU) {
		threshold      = 2.0f;
	}
	if (config.pinSizeThresholdLow > threshold) threshold = config.pinSizeThresholdLow;

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

		ImVec2 pos = CoordToScreen(pin->position.x, pin->position.y);
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
				if (psz < config.fontSize / 2) psz = config.fontSize / 2;
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
				if (psz < config.fontSize / 2) psz = config.fontSize / 2;
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
				if (psz < config.fontSize / 2) psz = config.fontSize / 2;
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
				if (pin->component->pins.size() >= static_cast<unsigned int>(config.pinA1threshold)) { // config.pinA1threshold is never negative
					color = fill_color = m_colors.pinA1PadColor;
					fill_pin           = m_colors.pinA1PadColor;
					draw_ring          = false;
				}
			}

			// don't show text if it doesn't make sense
			if (pin->component->pins.size() <= 1) show_text = false;
			if (pin->type == Pin::kPinTypeTestPad) show_text = false;
		}

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
			if ((show_text) && (psz < config.fontSize / 2)) psz = config.fontSize / 2;

			switch (pin->type) {
				case Pin::kPinTypeTestPad:
					if ((psz > 3) && (!config.slowCPU)) {
						draw->AddCircleFilled(ImVec2(pos.x, pos.y), psz, fill_color, segments);
						draw->AddCircle(ImVec2(pos.x, pos.y), psz, color, segments);
					} else if (psz > threshold) {
						draw->AddRectFilled(ImVec2(pos.x - h, pos.y - h), ImVec2(pos.x + h, pos.y + h), fill_color);
					}
					break;
				default:
					if ((psz > 3) && (psz > threshold)) {
						if (config.pinShapeSquare || config.slowCPU) {
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
			// config.pinHaloThickness);
			//		}

			// Show all pin names when config.showPinName is enabled and pin diameter is above threshold or show pin name only for selected part
			if ((config.showPinName && psz > 3) || show_text) {
				std::string text = pin->name + "\n" + pin->net->name;
				ImFont *font = ImGui::GetIO().Fonts->Fonts[0]; // Default font
				ImVec2 text_size_normalized = font->CalcTextSizeA(1.0f, FLT_MAX, 0.0f, text.c_str());

				float maxfontwidth = psz * 2.125/ text_size_normalized.x; // Fit horizontally with 6.75% overflow (should still avoid colliding with neighbours)
				float maxfontheight = psz * 1.5/ text_size_normalized.y; // Fit vertically with 25% top/bottom padding
				float maxfontsize = std::min(maxfontwidth, maxfontheight);

				// Font size for pin name only depends on height of text (rather than width of full text incl. net name) to scale to pin bounding box
				ImVec2 size_pin_name = font->CalcTextSizeA(maxfontheight, FLT_MAX, 0.0f, pin->name.c_str());
				// Font size for net name also depends on width of full text to avoid overflowing too much and colliding with text from other pin
				ImVec2 size_net_name = font->CalcTextSizeA(maxfontsize, FLT_MAX, 0.0f, pin->net->name.c_str());

				// Show pin name above net name, full text is centered vertically
				ImVec2 pos_pin_name   = ImVec2(pos.x - size_pin_name.x * 0.5f, pos.y - size_pin_name.y);
				ImVec2 pos_net_name   = ImVec2(pos.x - size_net_name.x * 0.5f, pos.y);

				ImFont *font_pin_name = font;
				if (maxfontheight < font->FontSize * 0.75) {
					font_pin_name = ImGui::GetIO().Fonts->Fonts[2]; // Use smaller font for pin name
				} else if (maxfontheight > font->FontSize * 1.5 && ImGui::GetIO().Fonts->Fonts[1]->FontSize > font->FontSize) {
					font_pin_name = ImGui::GetIO().Fonts->Fonts[1]; // Use larger font for pin name
				}

				ImFont *font_net_name = font;
				if (maxfontsize < font->FontSize * 0.75) {
					font_net_name = ImGui::GetIO().Fonts->Fonts[2]; // Use smaller font for net name
				} else if (maxfontsize > font->FontSize * 1.5 && ImGui::GetIO().Fonts->Fonts[1]->FontSize > font->FontSize) {
					font_net_name = ImGui::GetIO().Fonts->Fonts[1]; // Use larger font for net name
				}

				// Background rectangle
				draw->AddRectFilled(ImVec2(pos_net_name.x - m_scale * 0.5f, pos_net_name.y), // Begining of text with slight padding
										ImVec2(pos_net_name.x + size_net_name.x + m_scale * 0.5f, pos_net_name.y + size_net_name.y), // End of text with slight padding
										m_colors.pinTextBackgroundColor,
										m_scale * 0.5f/*rounding*/);

				draw->ChannelsSetCurrent(kChannelText);
				draw->AddText(font_pin_name, maxfontheight, pos_pin_name, text_color, pin->name.c_str());
				draw->AddText(font_net_name, maxfontsize, pos_net_name, text_color, pin->net->name.c_str());
				draw->ChannelsSetCurrent(kChannelPins);
			}
		}
	}
}

inline void BoardView::DrawParts(ImDrawList *draw) {
	// float psz = (float)m_pinDiameter * 0.5f * m_scale;
	double angle;
	double distance = 0;
	uint32_t color = m_colors.partOutlineColor;
	//	int rendered   = 0;
	char p0, p1; // first two characters of the part name, code-writing
	             // convenience more than anything else

	draw->ChannelsSetCurrent(kChannelPolylines);
	/*
	 * If a pin has been selected, we mask out the colour to
	 * enhance (relatively) the appearance of the pin(s)
	 */
	if (config.pinSelectMasks && ((m_pinSelected) || m_pinHighlighted.size())) {
		color = (m_colors.partOutlineColor & m_colors.selectedMaskParts) | m_colors.orMaskParts;
	}

	for (auto &part : m_board->Components()) {
		int pincount = 0;
		double min_x, min_y, max_x, max_y, aspect;
		std::vector<ImVec2> pva;
		std::array<ImVec2, 4> dbox; // default box, if there's nothing else claiming to render the part different.

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

				pva.push_back({pin->position.x, pin->position.y});

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
			part->centerpoint = ImVec2((max_x - min_x) / 2 + min_x, (max_y - min_y) / 2 + min_y);

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

				part->outline = dbox;
				part->outline_done = true;

				part->hull.clear();
				for (auto &pin : part->pins) {
					part->hull.push_back({pin->position.x, pin->position.y});
				}

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
					// Find our hull
					std::vector<ImVec2> hull = VHConvexHull(pva);

					// If we had a valid hull, then find the MBB for it
					if (hull.size() > 0) {
						part->hull = hull;

						std::array<ImVec2, 4> bbox = VHMBBCalculate(hull, pin_radius);
						part->outline = bbox;
						part->outline_done = true;

						/*
						 * Tighten the hull, removes any small angle segments
						 * such as a sequence of pins in a line, might be an overkill
						 */
						// hpc = TightenHull(hull, hpc, 0.1f);
					}
				} else {
					// if it wasn't at an odd angle, or wasn't large, or wasn't a connector,
					// just an ordinary
					// type part, then this is where we'll likely end up
					part->outline = dbox;
					part->outline_done = true;
				}
			}

			//			if (rendered == 0) {
			//				fprintf(stderr, "Part wasn't rendered (%s)\n", part->name.c_str());
			//			}

		} // if !outline_done

		if (!BoardElementIsVisible(part) && !PartIsHighlighted(part)) continue;

		if (part->outline_done) {

			/*
			 * Draw the bounding box for the part
			 */
			ImVec2 a, b, c, d;

			a = ImVec2(CoordToScreen(part->outline[0].x, part->outline[0].y));
			b = ImVec2(CoordToScreen(part->outline[1].x, part->outline[1].y));
			c = ImVec2(CoordToScreen(part->outline[2].x, part->outline[2].y));
			d = ImVec2(CoordToScreen(part->outline[3].x, part->outline[3].y));

			// if (config.fillParts) draw->AddQuadFilled(a, b, c, d, color & 0xffeeeeee);
			if (config.fillParts && !config.slowCPU) draw->AddQuadFilled(a, b, c, d, m_colors.partFillColor);
			draw->AddQuad(a, b, c, d, color);
			if (PartIsHighlighted(part)) {
				if (config.fillParts && !config.slowCPU) draw->AddQuadFilled(a, b, c, d, m_colors.partHighlightedFillColor);
				draw->AddQuad(a, b, c, d, m_colors.partHighlightedColor);
			}

			/*
			 * Draw the convex hull of the part if it has one
			 */
			if (!part->hull.empty()) {
				draw->PathClear();
				for (size_t i = 0; i < part->hull.size(); i++) {
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
					int segments = trunc(part->expanse);
					if (segments < 8) segments = 8;
					if (segments > 36) segments = 36;
					draw->AddCircle(CoordToScreen(part->centerpoint.x, part->centerpoint.y),
					                (part->expanse / 3) * m_scale,
					                m_colors.partOutlineColor & 0x8fffffff,
					                segments);
				}
			}

			if (!part->is_dummy() && !part->name.empty()) {
				std::string text  = part->name;

				/*
				 * Draw part name inside part bounding box
				 */
				if (config.showPartName) {
					ImFont *font = ImGui::GetIO().Fonts->Fonts[0]; // Default font
					ImVec2 text_size_normalized	= font->CalcTextSizeA(1.0f, FLT_MAX, 0.0f, text.c_str());

					// Find max width and height of bounding box, not perfect for non-straight bounding box but good enough
					float minx = std::min({a.x, b.x, c.x, d.x});
					float miny = std::min({a.y, b.y, c.y, d.y});
					float maxx = std::max({a.x, b.x, c.x, d.x});
					float maxy = std::max({a.y, b.y, c.y, d.y});
					float maxwidth = abs(maxx - minx) * 0.7; // Bounding box width with 30% padding
					float maxheight = abs(maxy - miny) * 0.7; // Bounding box height with 30% padding

					// Find max font size to fit text inside bounding box
					float maxfontwidth = maxwidth / text_size_normalized.x;
					float maxfontheight = maxheight / text_size_normalized.y;
					float maxfontsize = std::min(maxfontwidth, maxfontheight);

					ImVec2 text_size{text_size_normalized.x * maxfontsize, text_size_normalized.y * maxfontsize};

					// Center text
					ImVec2 pos = CoordToScreen(part->centerpoint.x, part->centerpoint.y); // Computed previously during bounding box generation
					pos.x -= text_size.x * 0.5f;
					pos.y -= text_size.y * 0.5f;

					if (maxfontsize < font->FontSize * 0.75) {
						font = ImGui::GetIO().Fonts->Fonts[2]; // Use smaller font for part name
					} else if (maxfontsize > font->FontSize * 1.5 && ImGui::GetIO().Fonts->Fonts[1]->FontSize > font->FontSize) {
						font = ImGui::GetIO().Fonts->Fonts[1]; // Use larger font for part name
					}

					draw->ChannelsSetCurrent(kChannelText);
					draw->AddText(font, maxfontsize, pos, m_colors.partTextColor, part->name.c_str());
					draw->ChannelsSetCurrent(kChannelPolylines);
				}

				/*
				 * Draw the highlighted text for selected part
				 */
				if (PartIsHighlighted(part)) {
					std::string mcode = part->mfgcode;

					ImVec2 text_size    = ImGui::CalcTextSize(text.c_str());
					ImVec2 mfgcode_size = ImGui::CalcTextSize(mcode.c_str());

					if ((!config.showInfoPanel) && (mfgcode_size.x > text_size.x)) text_size.x = mfgcode_size.x;

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
										m_colors.partHighlightedTextBackgroundColor,
										0.0f);
					draw->AddText(pos, m_colors.partHighlightedTextColor, text.c_str());
					if ((!config.showInfoPanel) && (mcode.size())) {
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
		}
	} // for each part
}

void BoardView::DrawPartTooltips(ImDrawList *draw) {
	ImVec2 spos = ImGui::GetMousePos();
	ImVec2 pos  = ScreenToCoord(spos.x, spos.y);

	if (!ImGui::IsWindowHovered()) return;

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

				draw->AddCircle(CoordToScreen(pin->position.x, pin->position.y), pd, m_colors.pinHaloColor, 32, config.pinHaloThickness);
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

	currentlyHoveredPart = nullptr;
	for (auto &part : m_board->Components()) {
		int hit = 0;
		//		auto p_part = part.get();


		// Work out if the point is inside the hull
		{
			auto poly = part->outline;

			for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
				if (((poly[i].y > pos.y) != (poly[j].y > pos.y)) &&
				    (pos.x < (poly[j].x - poly[i].x) * (pos.y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x))
					hit ^= 1;
			}
		}

		// If we're inside a part
		if (hit) {
			currentlyHoveredPart = part;
			//			fprintf(stderr,"InPart: %s\n", currentlyHoveredPart->name.c_str());

			float min_dist = m_pinDiameter / 2.0f;
			min_dist *= min_dist; // all distance squared
			currentlyHoveredPin = nullptr;

			for (auto &pin : currentlyHoveredPart->pins) {
				// auto p     = pin;
				float dx   = pin->position.x - pos.x;
				float dy   = pin->position.y - pos.y;
				float dist = dx * dx + dy * dy;

				if (!BoardElementIsVisible(pin)) {
					continue;
				}

				if ((dist < (pin->diameter * pin->diameter)) && (dist < min_dist)) {
					currentlyHoveredPin = pin;
					//					fprintf(stderr,"Pinhit: %s\n",pin->number.c_str());
					min_dist = dist;
				} // if in the required diameter
			}     // for each pin in the part

			if (!BoardElementIsVisible(part) && (!currentlyHoveredPin || !BoardElementIsVisible(currentlyHoveredPin))) {
				continue;
			}

			if (part->outline_done) {

				/*
				 * Draw the bounding box for the part
				 */
				ImVec2 a, b, c, d;

				a = ImVec2(CoordToScreen(part->outline[0].x, part->outline[0].y));
				b = ImVec2(CoordToScreen(part->outline[1].x, part->outline[1].y));
				c = ImVec2(CoordToScreen(part->outline[2].x, part->outline[2].y));
				d = ImVec2(CoordToScreen(part->outline[3].x, part->outline[3].y));
				draw->AddQuad(a, b, c, d, m_colors.partHighlightedColor, 2);
			}

			draw->ChannelsSetCurrent(kChannelAnnotations);

			if (currentlyHoveredPin)
				draw->AddCircle(CoordToScreen(currentlyHoveredPin->position.x, currentlyHoveredPin->position.y),
				                currentlyHoveredPin->diameter * m_scale,
				                m_colors.pinHaloColor,
				                32,
				                config.pinHaloThickness);
			ImGui::PushStyleColor(ImGuiCol_Text, m_colors.annotationPopupTextColor);
			ImGui::PushStyleColor(ImGuiCol_PopupBg, m_colors.annotationPopupBackgroundColor);
			ImGui::BeginTooltip();
			if (currentlyHoveredPin) {
				ImGui::Text("%s\n[%s]%s",
				            currentlyHoveredPart->name.c_str(),
				            (currentlyHoveredPin ? currentlyHoveredPin->name.c_str() : " "),
				            (currentlyHoveredPin ? currentlyHoveredPin->net->name.c_str() : " "),
				            (currentlyHoveredPin ? currentlyHoveredPin->comment.c_str() : " "));
			} else {
				ImGui::Text("%s", currentlyHoveredPart->name.c_str());
			}
			ImGui::EndTooltip();
			ImGui::PopStyleColor(2);
		}

	} // for each part on the board
}

inline void BoardView::DrawPinTooltips(ImDrawList *draw) {
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

	if (!config.showAnnotations) return;

	draw->ChannelsSetCurrent(kChannelAnnotations);

	for (auto &ann : m_annotations.annotations) {
		if (ann.side == m_current_side) {
			ImVec2 a, b, s;
			if (debug) fprintf(stderr, "%d:%d:%f %f: %s\n", ann.id, ann.side, ann.x, ann.y, ann.note.c_str());
			a = s = CoordToScreen(ann.x, ann.y);
			a.x += DPI(config.annotationBoxOffset);
			a.y -= DPI(config.annotationBoxOffset);
			b = ImVec2(a.x + DPI(config.annotationBoxSize), a.y - DPI(config.annotationBoxSize));

			if ((ann.hovered == true) && (ImGui::IsWindowHovered())) {
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

	if (!ImGui::IsWindowHovered()) return false;
	m_annotation_last_hovered = 0;

	for (auto &ann : m_annotations.annotations) {
		ImVec2 a = CoordToScreen(ann.x, ann.y);
		if ((mp.x > a.x + DPI(config.annotationBoxOffset)) && (mp.x < a.x + (DPI(config.annotationBoxOffset) + DPI(config.annotationBoxSize))) &&
		    (mp.y < a.y - DPI(config.annotationBoxOffset)) && (mp.y > a.y - (DPI(config.annotationBoxOffset) + DPI(config.annotationBoxSize)))) {
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
	std::vector<ImVec2> pl;
	if ((m_pinHighlighted.size()) < 3) return;
	for (auto &p : m_pinHighlighted) {
		pl.push_back(CoordToScreen(p->position.x, p->position.y));
	}
	std::vector<ImVec2> hull = VHConvexHull(pl);
	if (hull.size() > 3) {
		draw->AddConvexPolyFilled(hull.data(), hull.size(), 0x660000ff);
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

	// We draw the Parts before the Pins so that we can ascertain the needed pin
	// size for the parts based on the part/pad geometry and spacing. -Inflex
	// OutlineGenerateFill();
	//	DrawFill(draw);
	OutlineGenFillDraw(draw, config.boardFillSpacing, 1);
	DrawOutline(draw);
	DrawParts(draw);
	//	DrawSelectedPins(draw);
	DrawPins(draw);
	// DrawPinTooltips(draw);
	DrawPartTooltips(draw);
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

void BoardView::LoadBoard(BRDFileBase *file) {
	delete m_board;

	// Check board outline (format) point count.
	//		If we don't have an outline, generate one
	//
	if (file->outline_segments.size() < 3 && file->format.size() < 3) {
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

	m_board = new BRDBoard(file);
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
	for (auto &pa : m_board->OutlinePoints()) {
		if (pa->x < min_x) min_x = pa->x;
		if (pa->y < min_y) min_y = pa->y;
		if (pa->x > max_x) max_x = pa->x;
		if (pa->y > max_y) max_y = pa->y;
	}
	for (auto &s: m_board->OutlineSegments()) {
		if (s.first.x < min_x) min_x = s.first.x;
		if (s.second.x < min_x) min_x = s.second.x;
		if (s.first.y < min_y) min_y = s.first.y;
		if (s.second.y < min_y) min_y = s.second.y;
		if (s.first.x > max_x) max_x = s.first.x;
		if (s.second.x > max_x) max_x = s.second.x;
		if (s.first.y > max_y) max_y = s.first.y;
		if (s.second.y > max_y) max_y = s.second.y;
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
			for (size_t i = 0; i < part->outline.size(); i++) {
				part->outline[i].x = max.x - part->outline[i].x;
			}
		}

		for (size_t i = 0; i < part->hull.size(); i++) {
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

inline bool BoardView::BoardElementIsVisible(const std::shared_ptr<BoardElement> be) {
	if (!be) return true; // no element? => no board side info

	if (be->board_side == m_current_side) return true;

	if (be->board_side == kBoardSideBoth) return true;

	return false;
}

inline bool BoardView::IsVisibleScreen(float x, float y, float radius, const ImGuiIO &io) {
	// if (x < -radius || y < -radius || x - radius > io.DisplaySize.x || y - radius > io.DisplaySize.y) return false;
	if (x < -radius || y < -radius || x - radius > m_board_surface.x || y - radius > m_board_surface.y) return false;
	return true;
}

bool BoardView::PartIsHighlighted(const std::shared_ptr<Component> component) {
	bool highlighted = contains(component, m_partHighlighted);

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
	m_pinHighlighted.clear();
	m_partHighlighted.clear();
	//	ClearAllHighlights();
	if (*item == '\0') return;

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
		mode = config.flipMode;

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

void BoardView::HandlePDFBridgeSelection() {
	if (pdfBridge.HasNewSelection()) {
		auto selection = pdfBridge.GetSelection();
		if (selection.empty()) {
			m_pinHighlighted.clear();
			m_partHighlighted.clear();
		} else {
			SearchCompound(selection.c_str());
			CenterZoomSearchResults();
		}
	};
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
