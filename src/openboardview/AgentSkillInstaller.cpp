#include "AgentSkillInstaller.h"

#include "filesystem_impl.h"

#include <SDL.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

namespace {

filesystem::path user_home() {
	if (const char *override_path = std::getenv("OPENBOARDVIEW_AGENT_HOME")) {
		if (*override_path) return filesystem::u8path(override_path);
	}
	const char *home = std::getenv("HOME");
#ifdef _WIN32
	if (!home || !*home) home = std::getenv("USERPROFILE");
#endif
	return home && *home ? filesystem::u8path(home) : filesystem::path{};
}

filesystem::path bundled_skill() {
	if (const char *override_path = std::getenv("OPENBOARDVIEW_SKILL_DIR")) {
		if (*override_path) return filesystem::u8path(override_path);
	}

	char *raw_base = SDL_GetBasePath();
	if (!raw_base) return {};
	const filesystem::path base = filesystem::u8path(raw_base);
	SDL_free(raw_base);
#ifdef __APPLE__
	return base / ".." / "Resources" / "openboardview" / "skills" / "board-diagnostics";
#else
	return base / ".." / "share" / "openboardview" / "skills" / "board-diagnostics";
#endif
}

bool files_equal(const filesystem::path &left, const filesystem::path &right) {
	std::error_code error;
	if (!filesystem::is_regular_file(right, error) || error) return false;
	const auto left_size = filesystem::file_size(left, error);
	if (error) return false;
	const auto right_size = filesystem::file_size(right, error);
	if (error || left_size != right_size) return false;

	ifstream left_stream(left, std::ios::binary);
	ifstream right_stream(right, std::ios::binary);
	return left_stream && right_stream &&
	       std::equal(std::istreambuf_iterator<char>(left_stream),
	                  std::istreambuf_iterator<char>(),
	                  std::istreambuf_iterator<char>(right_stream));
}

bool sync_file(const filesystem::path &source, const filesystem::path &destination) {
	if (files_equal(source, destination)) return true;
	std::error_code error;
	filesystem::create_directories(destination.parent_path(), error);
	if (error) return false;
	filesystem::copy_file(source, destination, filesystem::copy_options::overwrite_existing, error);
	return !error;
}

void sync_skill(const filesystem::path &source, const filesystem::path &destination) {
	static const std::vector<filesystem::path> files = {
	    "SKILL.md",
	    filesystem::path("references") / "checklist-plan.md",
	    filesystem::path("scripts") / "board_diagnostics.py",
	    filesystem::path("scripts") / "board_diagnostics_core.py",
	};

	std::error_code error;
	const bool destination_exists = filesystem::exists(destination, error);
	const filesystem::path marker = destination / ".openboardview-managed";
	if (error || (destination_exists && !filesystem::exists(marker, error))) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
		            "Board Diagnostics skill already exists and is not managed by OpenBoardView: %s",
		            destination.string().c_str());
		return;
	}

	filesystem::create_directories(destination, error);
	if (error) return;
	if (!filesystem::exists(marker, error)) {
		ofstream marker_stream(marker);
		marker_stream << "Installed and updated by OpenBoardView.\n";
		if (!marker_stream) return;
	}

	for (const auto &relative : files) {
		if (!sync_file(source / relative, destination / relative)) {
			SDL_LogWarn(
			    SDL_LOG_CATEGORY_APPLICATION, "Unable to install Board Diagnostics skill file: %s", relative.string().c_str());
			return;
		}
	}
}

} // namespace

void InstallBundledBoardDiagnosticsSkill() {
	const filesystem::path source = bundled_skill();
	const filesystem::path home   = user_home();
	std::error_code error;
	if (home.empty() || !filesystem::is_regular_file(source / "SKILL.md", error) || error) return;

	// Codex and OpenCode both discover ~/.agents/skills. Claude Code uses ~/.claude/skills.
	sync_skill(source, home / ".agents" / "skills" / "board-diagnostics");
	sync_skill(source, home / ".claude" / "skills" / "board-diagnostics");
}
