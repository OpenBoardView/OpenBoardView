#pragma once

#include "filesystem_impl.h"

#include <chrono>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

struct json_object;

class DiagnosticsPanel {
public:
	using LocateCallback = std::function<bool(const std::string &, bool, std::string &)>;

	DiagnosticsPanel();
	~DiagnosticsPanel();

	DiagnosticsPanel(const DiagnosticsPanel &) = delete;
	DiagnosticsPanel &operator=(const DiagnosticsPanel &) = delete;

	void SetBoardFile(const filesystem::path &board_file);
	void SetPreferredTicket(const std::string &ticket);
	void SetPendingTarget(const std::string &target);

	// Polls ticket/request files. Returns true when external state requests attention.
	bool PollForChanges();
	bool HasCase() const;
	const std::string &Ticket() const;
	std::string TakePendingTarget();

	void Draw(const LocateCallback &locate);

private:
	struct CaseCandidate {
		filesystem::path path;
		std::string ticket;
		std::string board;
		std::string status;
		std::string updated_at;
	};

	filesystem::path board_file_;
	filesystem::path selected_case_path_;
	std::vector<CaseCandidate> candidates_;
	json_object *root_ = nullptr;
	std::unordered_map<std::string, std::string> edit_buffers_;
	std::string preferred_ticket_;
	std::string pending_target_;
	std::string ticket_;
	std::string notice_;
	std::string last_located_target_;
	bool notice_is_error_ = false;
	bool contextual_view_ = false;
	bool have_case_write_time_ = false;
	bool have_request_write_time_ = false;
	filesystem::file_time_type case_write_time_{};
	filesystem::file_time_type request_write_time_{};
	std::chrono::steady_clock::time_point next_poll_{};

	filesystem::path DataRoot() const;
	filesystem::path RequestPath() const;
	filesystem::path AcknowledgementPath() const;
	void AcknowledgeRequest(const std::string &request_id) const;
	void ClearCase();
	bool RefreshCandidates();
	bool LoadCase(const filesystem::path &path);
	bool LoadPreferredOrFirstCase();
	bool PollOpenRequest();
	bool RequestMatchesBoard(json_object *request) const;
	bool SaveCase();

	json_object *ActiveRound() const;
	std::string &EditableBuffer(const std::string &key, const std::string &initial);
	void MarkEdited(json_object *round);
	void ApplyDependencies(json_object *round);
	std::string DependencyState(json_object *round, json_object *measurement) const;
	void SetMeasurementStatus(json_object *round, json_object *measurement, const std::string &status);
	void SetMeasurementActual(json_object *round, json_object *measurement, const std::string &actual);
	void SetSideComments(json_object *round, json_object *side, const std::string &comments);
	void FinishRound(json_object *round);

	void DrawTicketPicker();
	void DrawRound(json_object *round, const LocateCallback &locate);
	void DrawSide(json_object *round, json_object *side, const LocateCallback &locate);
	void DrawSection(json_object *round, json_object *section, const LocateCallback &locate);
	void DrawMeasurement(json_object *round, json_object *measurement, const LocateCallback &locate);
};
