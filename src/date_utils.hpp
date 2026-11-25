#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace tracker {

struct DateYMD {
    int year = 0;
    int month = 0;
    int day = 0;
};

/// Returns today's local date formatted as "YYYY-MM-DD".
[[nodiscard]] std::string get_current_date_iso();

/// Parses an ISO "YYYY-MM-DD" date string into its year/month/day components.
[[nodiscard]] std::optional<DateYMD> parse_date_iso(std::string_view date_iso);

/// Calculates the signed number of days from today until target_date_iso.
[[nodiscard]] int get_days_until(std::string_view target_date_iso);

/// Formats a human-readable Italian countdown string relative to exam_date_iso.
[[nodiscard]] std::string format_countdown(std::string_view exam_date_iso);

/// Returns true if date_iso falls within the current ISO calendar week (Monday..Sunday).
[[nodiscard]] bool is_in_current_week(std::string_view date_iso);

} // namespace tracker
