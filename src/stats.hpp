#pragma once

#include "model.hpp"
#include <string>
#include <ftxui/screen/color.hpp>

namespace tracker {

/// Counts blocks completed during the current ISO calendar week.
[[nodiscard]] int count_weekly_completed_blocks(const Roadmap& roadmap);

/// Counts total completed blocks across the entire roadmap.
[[nodiscard]] int count_total_completed_blocks(const Roadmap& roadmap);

/// Formats a duration in seconds as "MM:SS".
[[nodiscard]] std::string format_time_mm_ss(int total_seconds);

/// Formats a duration in seconds as "Xh Ym" or "Ym".
[[nodiscard]] std::string format_duration_hm(int total_seconds);

/// Maps a study phase index to its badge color.
[[nodiscard]] ftxui::Color get_phase_color(int phase);

} // namespace tracker
