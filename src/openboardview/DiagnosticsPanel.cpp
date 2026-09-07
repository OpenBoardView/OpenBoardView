#include "DiagnosticsPanel.h"

#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

#include <json-c/json.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <unordered_map>

#ifndef _WIN32
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace {

json_object *field(json_object *object, const char *name) {
	json_object *value = nullptr;
	if (!object || !json_object_is_type(object, json_type_object)) return nullptr;
	return json_object_object_get_ex(object, name, &value) ? value : nullptr;
}

std::string text_field(json_object *object, const char *name) {
	json_object *value = field(object, name);
	if (!value || !json_object_is_type(value, json_type_string)) return {};
	const char *text = json_object_get_string(value);
	return text ? text : "";
}

bool bool_field(json_object *object, const char *name, bool fallback = false) {
	json_object *value = field(object, name);
	return value ? json_object_get_boolean(value) != 0 : fallback;
}

void set_text(json_object *object, const char *name, const std::string &value) {
	json_object_object_add(object, name, json_object_new_string(value.c_str()));
}

void set_bool(json_object *object, const char *name, bool value) {
	json_object_object_add(object, name, json_object_new_boolean(value));
}

std::string lowercase(std::string value) {
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return value;
}

std::string timestamp_utc() {
	const std::time_t now = std::time(nullptr);
	std::tm utc{};
#ifdef _WIN32
	gmtime_s(&utc, &now);
#else
	gmtime_r(&now, &utc);
#endif
	std::ostringstream output;
	output << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
	return output.str();
}

filesystem::path normalized_path(const filesystem::path &path) {
	if (path.empty()) return {};
	std::error_code error;
	auto normalized = filesystem::weakly_canonical(path, error);
	return error ? path.lexically_normal() : normalized;
}

bool path_is_within(const filesystem::path &path, const filesystem::path &directory) {
	const auto child = normalized_path(path);
	const auto parent = normalized_path(directory);
	if (child.empty() || parent.empty()) return false;
	auto child_it = child.begin();
	for (auto parent_it = parent.begin(); parent_it != parent.end(); ++parent_it, ++child_it) {
		if (child_it == child.end() || *child_it != *parent_it) return false;
	}
	return true;
}

template<typename Callback>
void each_measurement(json_object *round, Callback callback) {
	json_object *sides = field(round, "sides");
	if (!sides || !json_object_is_type(sides, json_type_array)) return;
	for (size_t side_index = 0; side_index < json_object_array_length(sides); ++side_index) {
		json_object *side = json_object_array_get_idx(sides, side_index);
		json_object *sections = field(side, "sections");
		if (!sections || !json_object_is_type(sections, json_type_array)) continue;
		for (size_t section_index = 0; section_index < json_object_array_length(sections); ++section_index) {
			json_object *section = json_object_array_get_idx(sections, section_index);
			json_object *measurements = field(section, "measurements");
			if (!measurements || !json_object_is_type(measurements, json_type_array)) continue;
			for (size_t measurement_index = 0; measurement_index < json_object_array_length(measurements); ++measurement_index) {
				callback(json_object_array_get_idx(measurements, measurement_index));
			}
		}
	}
}

struct Progress {
	int total = 0;
	int measured = 0;
	int failed = 0;
	int not_measurable = 0;
	int skipped = 0;
};

Progress progress_for(json_object *round) {
	Progress progress;
	each_measurement(round, [&](json_object *measurement) {
		++progress.total;
		const std::string status = text_field(measurement, "status");
		if (status != "untested" && !status.empty()) ++progress.measured;
		if (status == "fail") ++progress.failed;
		if (status == "not_measurable") ++progress.not_measurable;
		if (status == "skipped") ++progress.skipped;
	});
	return progress;
}

void disabled_wrapped(const std::string &text) {
	ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
	ImGui::TextWrapped("%s", text.c_str());
	ImGui::PopStyleColor();
}

} // namespace

DiagnosticsPanel::DiagnosticsPanel() : next_poll_(std::chrono::steady_clock::now()) {}

DiagnosticsPanel::~DiagnosticsPanel() {
	ClearCase();
}

filesystem::path DiagnosticsPanel::DataRoot() const {
	if (const char *override_path = std::getenv("BOARD_DIAGNOSTICS_DATA_DIR")) {
		if (*override_path) return normalized_path(filesystem::u8path(override_path));
	}
	filesystem::path base;
	if (const char *xdg_data = std::getenv("XDG_DATA_HOME")) {
		if (*xdg_data) base = filesystem::u8path(xdg_data);
	}
	if (base.empty()) {
		const char *home = std::getenv("HOME");
#ifdef _WIN32
		if (!home || !*home) home = std::getenv("USERPROFILE");
#endif
		if (!home || !*home) return {};
		base = filesystem::u8path(home) / ".local" / "share";
	}
	return base / "board-diagnostics" / "tickets";
}

filesystem::path DiagnosticsPanel::RequestPath() const {
	const auto root = DataRoot();
	return root.empty() ? filesystem::path{} : root.parent_path() / "open-request.json";
}

filesystem::path DiagnosticsPanel::AcknowledgementPath() const {
	const auto root = DataRoot();
	return root.empty() ? filesystem::path{} : root.parent_path() / "open-request-ack.json";
}

void DiagnosticsPanel::AcknowledgeRequest(const std::string &request_id) const {
	const auto path = AcknowledgementPath();
	if (path.empty() || request_id.empty()) return;
	json_object *acknowledgement = json_object_new_object();
	set_text(acknowledgement, "request_id", request_id);
	set_text(acknowledgement, "ticket", ticket_);
	set_text(acknowledgement, "acknowledged_at", timestamp_utc());
	json_object_to_file_ext(path.string().c_str(), acknowledgement, JSON_C_TO_STRING_PRETTY);
	json_object_put(acknowledgement);
}

void DiagnosticsPanel::ClearCase() {
	if (root_) json_object_put(root_);
	root_ = nullptr;
	selected_case_path_.clear();
	ticket_.clear();
	last_located_target_.clear();
	contextual_view_ = false;
	edit_buffers_.clear();
	have_case_write_time_ = false;
}

void DiagnosticsPanel::SetPreferredTicket(const std::string &ticket) {
	preferred_ticket_ = ticket;
	if (!candidates_.empty()) LoadPreferredOrFirstCase();
}

void DiagnosticsPanel::SetPendingTarget(const std::string &target) {
	pending_target_ = target;
}

void DiagnosticsPanel::SetBoardFile(const filesystem::path &board_file) {
	board_file_ = normalized_path(board_file);
	RefreshCandidates();
	LoadPreferredOrFirstCase();
}

bool DiagnosticsPanel::RefreshCandidates() {
	std::vector<CaseCandidate> discovered;
	const auto root = DataRoot();
	std::error_code error;
	if (!root.empty() && filesystem::is_directory(root, error)) {
		for (const auto &entry : filesystem::directory_iterator(root, error)) {
			if (error || !entry.is_directory()) continue;
			const auto case_path = entry.path() / "case.json";
			json_object *case_object = json_object_from_file(case_path.string().c_str());
			if (!case_object) continue;

			const auto workspace = text_field(case_object, "source_workspace");
			const bool matches_board = board_file_.empty() ||
			                           (!workspace.empty() && path_is_within(board_file_, filesystem::u8path(workspace)));
			if (matches_board) {
				CaseCandidate candidate;
				candidate.path = normalized_path(case_path);
				candidate.ticket = text_field(case_object, "ticket");
				candidate.board = text_field(case_object, "board");
				candidate.status = text_field(case_object, "status");
				candidate.updated_at = text_field(case_object, "updated_at");
				if (!candidate.ticket.empty()) discovered.push_back(std::move(candidate));
			}
			json_object_put(case_object);
		}
	}
	std::sort(discovered.begin(), discovered.end(), [](const CaseCandidate &left, const CaseCandidate &right) {
		if (left.updated_at != right.updated_at) return left.updated_at > right.updated_at;
		return left.ticket < right.ticket;
	});

	const bool changed = discovered.size() != candidates_.size() ||
	                     !std::equal(discovered.begin(), discovered.end(), candidates_.begin(), [](const auto &left, const auto &right) {
		                     return left.path == right.path && left.updated_at == right.updated_at && left.status == right.status;
	                     });
	candidates_ = std::move(discovered);
	return changed;
}

bool DiagnosticsPanel::LoadPreferredOrFirstCase() {
	if (candidates_.empty()) {
		ClearCase();
		return false;
	}
	auto selected = candidates_.begin();
	if (!preferred_ticket_.empty()) {
		auto preferred = std::find_if(candidates_.begin(), candidates_.end(), [&](const CaseCandidate &candidate) {
			return lowercase(candidate.ticket) == lowercase(preferred_ticket_);
		});
		if (preferred != candidates_.end()) selected = preferred;
	} else {
		auto active = std::find_if(candidates_.begin(), candidates_.end(), [](const CaseCandidate &candidate) {
			return candidate.status == "in_progress" || candidate.status == "waiting_for_ai";
		});
		if (active != candidates_.end()) selected = active;
	}
	if (root_ && normalized_path(selected_case_path_) == normalized_path(selected->path)) return true;
	return LoadCase(selected->path);
}

bool DiagnosticsPanel::LoadCase(const filesystem::path &path) {
	json_object *loaded = json_object_from_file(path.string().c_str());
	if (!loaded || !json_object_is_type(loaded, json_type_object)) {
		if (loaded) json_object_put(loaded);
		notice_ = std::string("Could not load diagnostic ticket: ") + json_util_get_last_err();
		notice_is_error_ = true;
		return false;
	}
	const std::string loaded_ticket = text_field(loaded, "ticket");
	if (loaded_ticket.empty() || !field(loaded, "rounds")) {
		json_object_put(loaded);
		notice_ = "Diagnostic case is missing its ticket or rounds.";
		notice_is_error_ = true;
		return false;
	}

	if (root_) json_object_put(root_);
	root_ = loaded;
	selected_case_path_ = normalized_path(path);
	ticket_ = loaded_ticket;
	preferred_ticket_ = loaded_ticket;
	edit_buffers_.clear();
	notice_.clear();
	notice_is_error_ = false;
	std::error_code error;
	case_write_time_ = filesystem::last_write_time(selected_case_path_, error);
	have_case_write_time_ = !error;
	return true;
}

bool DiagnosticsPanel::RequestMatchesBoard(json_object *request) const {
	const std::string requested_board = text_field(request, "board_file");
	if (requested_board.empty() || board_file_.empty()) return false;
	return normalized_path(filesystem::u8path(requested_board)) == board_file_;
}

bool DiagnosticsPanel::PollOpenRequest() {
	if (board_file_.empty()) return false;
	const auto request_path = RequestPath();
	if (request_path.empty()) return false;
	std::error_code error;
	const auto write_time = filesystem::last_write_time(request_path, error);
	if (error || (have_request_write_time_ && write_time == request_write_time_)) return false;
	request_write_time_ = write_time;
	have_request_write_time_ = true;

	json_object *request = json_object_from_file(request_path.string().c_str());
	if (!request) return false;
	const bool matches = RequestMatchesBoard(request);
	bool handled = false;
	if (matches) {
		preferred_ticket_ = text_field(request, "ticket");
		pending_target_ = text_field(request, "target");
		RefreshCandidates();
		handled = LoadPreferredOrFirstCase();
		if (handled) AcknowledgeRequest(text_field(request, "request_id"));
	}
	json_object_put(request);
	return handled;
}

bool DiagnosticsPanel::PollForChanges() {
	const auto now = std::chrono::steady_clock::now();
	if (now < next_poll_) return false;
	next_poll_ = now + std::chrono::milliseconds(250);

	bool attention = PollOpenRequest();
	const bool candidates_changed = RefreshCandidates();
	if (!root_ && !candidates_.empty()) attention |= LoadPreferredOrFirstCase();
	if (root_ && candidates_changed) {
		auto selected = std::find_if(candidates_.begin(), candidates_.end(), [&](const CaseCandidate &candidate) {
			return candidate.path == selected_case_path_;
		});
		if (selected == candidates_.end()) {
			attention |= LoadPreferredOrFirstCase();
		}
	}

	if (root_ && !selected_case_path_.empty()) {
		std::error_code error;
		const auto write_time = filesystem::last_write_time(selected_case_path_, error);
		if (!error && (!have_case_write_time_ || write_time != case_write_time_)) {
			attention |= LoadCase(selected_case_path_);
		}
	}
	return attention;
}

bool DiagnosticsPanel::HasCase() const {
	return root_ != nullptr;
}

const std::string &DiagnosticsPanel::Ticket() const {
	return ticket_;
}

std::string DiagnosticsPanel::TakePendingTarget() {
	std::string target = pending_target_;
	pending_target_.clear();
	return target;
}

json_object *DiagnosticsPanel::ActiveRound() const {
	if (!root_) return nullptr;
	json_object *rounds = field(root_, "rounds");
	if (!rounds || !json_object_is_type(rounds, json_type_array)) return nullptr;
	const std::string active_id = text_field(root_, "active_round_id");
	json_object *fallback = nullptr;
	for (size_t index = 0; index < json_object_array_length(rounds); ++index) {
		json_object *round = json_object_array_get_idx(rounds, index);
		fallback = round;
		if (!active_id.empty() && text_field(round, "id") == active_id) return round;
	}
	return fallback;
}

std::string &DiagnosticsPanel::EditableBuffer(const std::string &key, const std::string &initial) {
	auto [entry, inserted] = edit_buffers_.try_emplace(key, initial);
	return entry->second;
}

bool DiagnosticsPanel::SaveCase() {
	if (!root_ || selected_case_path_.empty()) return false;
	set_text(root_, "updated_at", timestamp_utc());
	const char *serialized = json_object_to_json_string_ext(root_, JSON_C_TO_STRING_PRETTY);
	if (!serialized) {
		notice_ = "Could not serialize the diagnostic ticket.";
		notice_is_error_ = true;
		return false;
	}

	auto temporary = selected_case_path_;
#ifdef _WIN32
	temporary += ".tmp";
#else
	temporary += ".tmp." + std::to_string(getpid());
#endif
	{
		std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
		if (!output) {
			notice_ = "Could not create the temporary diagnostic file.";
			notice_is_error_ = true;
			return false;
		}
		output << serialized << '\n';
		output.flush();
		if (!output) {
			notice_ = "Could not write the diagnostic ticket.";
			notice_is_error_ = true;
			return false;
		}
	}
#ifndef _WIN32
	chmod(temporary.string().c_str(), S_IRUSR | S_IWUSR);
#endif
	std::error_code error;
	filesystem::rename(temporary, selected_case_path_, error);
	if (error) {
		filesystem::remove(temporary, error);
		notice_ = "Could not replace the diagnostic ticket file.";
		notice_is_error_ = true;
		return false;
	}
	case_write_time_ = filesystem::last_write_time(selected_case_path_, error);
	have_case_write_time_ = !error;
	notice_is_error_ = false;
	return true;
}

void DiagnosticsPanel::MarkEdited(json_object *round) {
	const std::string now = timestamp_utc();
	set_text(round, "updated_at", now);
	if (text_field(round, "status") == "complete") {
		set_text(round, "status", "in_progress");
		json_object_object_add(round, "completed_at", json_object_new_null());
		set_text(root_, "status", "in_progress");
	}
}

std::string DiagnosticsPanel::DependencyState(json_object *round, json_object *measurement) const {
	json_object *requirements = field(measurement, "requires_pass");
	if (!requirements || !json_object_is_type(requirements, json_type_array) || json_object_array_length(requirements) == 0) {
		return "ready";
	}

	std::unordered_map<std::string, json_object *> by_key;
	each_measurement(round, [&](json_object *candidate) {
		std::string key = text_field(candidate, "key");
		if (key.empty()) key = text_field(candidate, "id");
		if (!key.empty()) by_key[key] = candidate;
	});

	bool all_passed = true;
	for (size_t index = 0; index < json_object_array_length(requirements); ++index) {
		json_object *required = json_object_array_get_idx(requirements, index);
		const char *key_text = required ? json_object_get_string(required) : nullptr;
		if (!key_text || !by_key.count(key_text)) return "pending";
		const std::string status = text_field(by_key[key_text], "status");
		if (status == "fail" || status == "not_measurable" || status == "skipped") return "skipped";
		if (status != "pass") all_passed = false;
	}
	return all_passed ? "ready" : "pending";
}

void DiagnosticsPanel::ApplyDependencies(json_object *round) {
	bool changed = true;
	while (changed) {
		changed = false;
		each_measurement(round, [&](json_object *measurement) {
			const std::string state = DependencyState(round, measurement);
			const std::string status = text_field(measurement, "status");
			const bool auto_skipped = bool_field(measurement, "auto_skipped");
			if (state == "skipped" && (status == "untested" || status == "skipped" || status.empty())) {
				if (status != "skipped" || !auto_skipped) {
					set_text(measurement, "status", "skipped");
					set_bool(measurement, "auto_skipped", true);
					changed = true;
				}
			} else if (state != "skipped" && auto_skipped) {
				set_text(measurement, "status", "untested");
				set_bool(measurement, "auto_skipped", false);
				changed = true;
			}
		});
	}
}

void DiagnosticsPanel::SetMeasurementStatus(json_object *round, json_object *measurement, const std::string &status) {
	set_text(measurement, "status", status);
	set_bool(measurement, "auto_skipped", false);
	ApplyDependencies(round);
	MarkEdited(round);
	SaveCase();
}

void DiagnosticsPanel::SetMeasurementActual(json_object *round, json_object *measurement, const std::string &actual) {
	set_text(measurement, "actual", actual);
	MarkEdited(round);
	SaveCase();
}

void DiagnosticsPanel::SetSideComments(json_object *round, json_object *side, const std::string &comments) {
	set_text(side, "comments", comments);
	MarkEdited(round);
	SaveCase();
}

void DiagnosticsPanel::FinishRound(json_object *round) {
	const Progress progress = progress_for(round);
	if (progress.total == 0 || progress.measured != progress.total) {
		notice_ = "Resolve every required measurement before finishing the round.";
		notice_is_error_ = true;
		return;
	}
	const std::string now = timestamp_utc();
	set_text(round, "status", "complete");
	set_text(round, "updated_at", now);
	set_text(round, "completed_at", now);
	set_text(root_, "status", "waiting_for_ai");
	if (SaveCase()) {
		notice_ = "Round finished. Return to your AI assistant for the next round.";
		notice_is_error_ = false;
	}
}

void DiagnosticsPanel::DrawTicketPicker() {
	if (candidates_.size() <= 1) {
		ImGui::Text("Ticket %s", ticket_.c_str());
		return;
	}
	const std::string label = "Ticket " + ticket_;
	if (ImGui::BeginCombo("##diagnostic-ticket", label.c_str())) {
		for (const auto &candidate : candidates_) {
			const bool selected = candidate.path == selected_case_path_;
			const std::string item = candidate.ticket + " - " + candidate.board;
			if (ImGui::Selectable(item.c_str(), selected)) LoadCase(candidate.path);
			if (selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
}

void DiagnosticsPanel::DrawMeasurement(json_object *round, json_object *measurement, const LocateCallback &locate) {
	ImGui::PushID(measurement);
	const bool visible = ImGui::BeginChild("##measurement",
	                                       ImVec2(-1.0f, 0.0f),
	                                       ImGuiChildFlags_FrameStyle | ImGuiChildFlags_AutoResizeY);
	if (!visible) {
		ImGui::EndChild();
		ImGui::PopID();
		return;
	}

	const std::string point = text_field(measurement, "point");
	const std::string reference = text_field(measurement, "reference");
	const std::string target = text_field(measurement, "boardview");
	ImGui::TextWrapped("%s -> %s", point.c_str(), reference.c_str());
	if (!target.empty()) {
		if (ImGui::Button("Boardview", ImVec2(-1.0f, 0.0f))) {
			const bool same_target = lowercase(last_located_target_) == lowercase(target);
			const bool show_context = same_target ? !contextual_view_ : false;
			std::string error;
			if (locate(target, show_context, error)) {
				last_located_target_ = target;
				contextual_view_ = show_context;
				notice_ = show_context ? "Showing " + target + " in board context."
				                           : "Located " + target + ".";
				notice_is_error_ = false;
			} else {
				notice_ = error.empty() ? "The board target could not be located." : error;
				notice_is_error_ = true;
			}
		}
		if (ImGui::IsItemHovered()) {
			const bool same_target = lowercase(last_located_target_) == lowercase(target);
			ImGui::SetTooltip("%s", same_target && !contextual_view_ ? "Click again to zoom out" : "Show on the board");
		}
	}

	const std::string expected = text_field(measurement, "expected");
	ImGui::TextDisabled("Expected");
	ImGui::TextWrapped("%s", expected.c_str());
	const std::string probe = text_field(measurement, "probe");
	if (!probe.empty()) {
		ImGui::TextDisabled("Probe placement");
		ImGui::TextWrapped("%s", probe.c_str());
	}
	const std::string location = text_field(measurement, "location");
	if (!location.empty()) ImGui::TextDisabled("%s", location.c_str());
	const std::string why = text_field(measurement, "why");
	if (!why.empty() && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", why.c_str());

	const std::string dependency = DependencyState(round, measurement);
	const std::string status = text_field(measurement, "status").empty() ? "untested" : text_field(measurement, "status");
	const bool locked = dependency != "ready" && (status == "untested" || status == "skipped");
	if (dependency == "pending") ImGui::TextDisabled("Waiting for prerequisite measurements");
	if (dependency == "skipped") ImGui::TextDisabled("Skipped because a prerequisite did not pass");

	ImGui::BeginDisabled(locked);
	const std::string buffer_key = text_field(round, "id") + "/" + text_field(measurement, "id") + "/actual";
	std::string &actual = EditableBuffer(buffer_key, text_field(measurement, "actual"));
	ImGui::TextDisabled("Actual");
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputText("##actual", &actual)) SetMeasurementActual(round, measurement, actual);

	const float status_spacing = ImGui::GetStyle().ItemSpacing.x;
	const float status_width = (ImGui::GetContentRegionAvail().x - status_spacing * 2.0f) / 3.0f;
	auto status_button = [&](const char *label, const char *value, const ImVec4 &color) {
		const bool active = status == value;
		if (active) {
			ImVec4 active_color = color;
			active_color.w = 0.55f;
			ImGui::PushStyleColor(ImGuiCol_Button, active_color);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, active_color);
		}
		if (ImGui::Button(label, ImVec2(status_width, 0.0f))) SetMeasurementStatus(round, measurement, active ? "untested" : value);
		if (active) ImGui::PopStyleColor(2);
	};
	const ImGuiStyle &style = ImGui::GetStyle();
	status_button("Pass##status", "pass", style.Colors[ImGuiCol_CheckMark]);
	ImGui::SameLine();
	status_button("Wrong##status", "fail", style.Colors[ImGuiCol_PlotLinesHovered]);
	ImGui::SameLine();
	status_button("N/A##status", "not_measurable", style.Colors[ImGuiCol_TextDisabled]);
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Can't measure");
	ImGui::EndDisabled();
	ImGui::EndChild();
	ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * 0.35f));
	ImGui::PopID();
}

void DiagnosticsPanel::DrawSection(json_object *round, json_object *section, const LocateCallback &locate) {
	std::string heading = text_field(section, "title");
	const std::string mode = text_field(section, "mode");
	const std::string power_state = text_field(section, "power_state");
	if (!mode.empty() || !power_state.empty()) heading += " [" + mode + ", power " + power_state + "]";
	ImGui::Separator();
	ImGui::TextWrapped("%s", heading.c_str());
	const std::string safety = text_field(section, "safety");
	if (!safety.empty()) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogramHovered));
		ImGui::TextWrapped("%s", safety.c_str());
		ImGui::PopStyleColor();
	}

	json_object *measurements = field(section, "measurements");
	if (!measurements || !json_object_is_type(measurements, json_type_array)) return;
	for (size_t index = 0; index < json_object_array_length(measurements); ++index) {
		DrawMeasurement(round, json_object_array_get_idx(measurements, index), locate);
	}
}

void DiagnosticsPanel::DrawSide(json_object *round, json_object *side, const LocateCallback &locate) {
	json_object *sections = field(side, "sections");
	if (sections && json_object_is_type(sections, json_type_array)) {
		for (size_t index = 0; index < json_object_array_length(sections); ++index) {
			DrawSection(round, json_object_array_get_idx(sections, index), locate);
		}
	}

	ImGui::SeparatorText("Comments");
	const std::string key = text_field(round, "id") + "/" + text_field(side, "id") + "/comments";
	std::string &comments = EditableBuffer(key, text_field(side, "comments"));
	const float comments_height = ImGui::GetTextLineHeightWithSpacing() * 4.5f;
	if (ImGui::InputTextMultiline("##side-comments", &comments, ImVec2(-1.0f, comments_height))) SetSideComments(round, side, comments);
}

void DiagnosticsPanel::DrawRound(json_object *round, const LocateCallback &locate) {
	const std::string heading = "Round " + std::to_string(json_object_get_int(field(round, "number"))) + ": " +
	                            text_field(round, "title");
	ImGui::Separator();
	ImGui::TextWrapped("%s", heading.c_str());
	const std::string summary = text_field(round, "summary");
	if (!summary.empty()) ImGui::TextWrapped("%s", summary.c_str());

	json_object *instructions = field(round, "instructions");
	if (instructions && json_object_is_type(instructions, json_type_array)) {
		for (size_t index = 0; index < json_object_array_length(instructions); ++index) {
			json_object *instruction = json_object_array_get_idx(instructions, index);
			ImGui::Bullet();
			ImGui::SameLine();
			ImGui::TextWrapped("%s", instruction ? json_object_get_string(instruction) : "");
		}
	}

	json_object *sides = field(round, "sides");
	if (sides && json_object_is_type(sides, json_type_array) && ImGui::BeginTabBar("##diagnostic-sides")) {
		for (size_t index = 0; index < json_object_array_length(sides); ++index) {
			json_object *side = json_object_array_get_idx(sides, index);
			const std::string name = text_field(side, "name");
			if (ImGui::BeginTabItem(name.c_str())) {
				DrawSide(round, side, locate);
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}

	const Progress progress = progress_for(round);
	ImGui::SeparatorText("Progress");
	const std::string progress_text = std::to_string(progress.measured) + " of " + std::to_string(progress.total) + " checked";
	const float progress_fraction = progress.total > 0 ? static_cast<float>(progress.measured) / progress.total : 0.0f;
	ImGui::ProgressBar(progress_fraction, ImVec2(-1.0f, 0.0f), progress_text.c_str());
	if (progress.failed || progress.not_measurable || progress.skipped) {
		ImGui::TextDisabled("%d wrong  |  %d not measurable  |  %d skipped",
		                    progress.failed,
		                    progress.not_measurable,
		                    progress.skipped);
	}
	const bool complete = text_field(round, "status") == "complete";
	ImGui::BeginDisabled(complete || progress.total == 0 || progress.measured != progress.total);
	if (ImGui::Button(complete ? "Finished" : "Finish round", ImVec2(-1.0f, 0.0f))) FinishRound(round);
	ImGui::EndDisabled();
}

void DiagnosticsPanel::Draw(const LocateCallback &locate) {
	PollForChanges();
	if (!root_) {
		ImGui::TextWrapped("No diagnostic ticket matches this board file.");
		if (ImGui::Button("Refresh tickets")) {
			RefreshCandidates();
			LoadPreferredOrFirstCase();
		}
		return;
	}

	DrawTicketPicker();
	const std::string board = text_field(root_, "board");
	if (!board.empty()) disabled_wrapped(board);
	if (json_object *round = ActiveRound()) DrawRound(round, locate);
	else ImGui::TextWrapped("This ticket has no active round.");

	if (!notice_.empty()) {
		const ImVec4 color = ImGui::GetStyleColorVec4(notice_is_error_ ? ImGuiCol_PlotLinesHovered : ImGuiCol_CheckMark);
		ImGui::TextColored(color, "%s", notice_.c_str());
	}
}
