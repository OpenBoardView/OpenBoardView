#pragma once

#include <chrono>
#include <cstdint>
#include <string>

class SystemTheme {
public:
	bool ApplyIfChanged(bool force = false);

private:
	std::string ThemePath() const;

	std::chrono::steady_clock::time_point next_check_{};
	std::int64_t modified_at_ = 0;
	std::int64_t file_size_ = -1;
	bool applied_ = false;
};
