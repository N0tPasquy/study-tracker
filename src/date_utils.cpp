#include "date_utils.hpp"

#include <charconv>
#include <cmath>
#include <cstdio>
#include <ctime>

namespace tracker {

namespace {

std::tm get_local_tm(std::time_t t) {
    std::tm result{};
#if defined(_WIN32)
    localtime_s(&result, &t);
#else
    localtime_r(&t, &result);
#endif
    return result;
}

bool parse_fixed_int(std::string_view sv, int& out) {
    const char* begin = sv.data();
    const char* end = begin + sv.size();
    auto [ptr, ec] = std::from_chars(begin, end, out);
    return ec == std::errc{} && ptr == end;
}

} // namespace

std::string get_current_date_iso() {
    std::tm tm_now = get_local_tm(std::time(nullptr));
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                  tm_now.tm_year + 1900,
                  tm_now.tm_mon + 1,
                  tm_now.tm_mday);
    return std::string(buf);
}

std::optional<DateYMD> parse_date_iso(std::string_view date_iso) {
    if (date_iso.size() != 10 || date_iso[4] != '-' || date_iso[7] != '-') {
        return std::nullopt;
    }

    DateYMD ymd{};
    if (!parse_fixed_int(date_iso.substr(0, 4), ymd.year) ||
        !parse_fixed_int(date_iso.substr(5, 2), ymd.month) ||
        !parse_fixed_int(date_iso.substr(8, 2), ymd.day)) {
        return std::nullopt;
    }

    if (ymd.month < 1 || ymd.month > 12 || ymd.day < 1 || ymd.day > 31) {
        return std::nullopt;
    }

    return ymd;
}

int get_days_until(std::string_view target_date_iso) {
    auto parsed = parse_date_iso(target_date_iso);
    if (!parsed) {
        return 0;
    }

    std::tm tm_now = get_local_tm(std::time(nullptr));

    std::tm tm_today{};
    tm_today.tm_year = tm_now.tm_year;
    tm_today.tm_mon = tm_now.tm_mon;
    tm_today.tm_mday = tm_now.tm_mday;
    tm_today.tm_hour = 12;
    tm_today.tm_isdst = -1;

    std::tm tm_target{};
    tm_target.tm_year = parsed->year - 1900;
    tm_target.tm_mon = parsed->month - 1;
    tm_target.tm_mday = parsed->day;
    tm_target.tm_hour = 12;
    tm_target.tm_isdst = -1;

    std::time_t time_today = std::mktime(&tm_today);
    std::time_t time_target = std::mktime(&tm_target);

    if (time_today == -1 || time_target == -1) {
        return 0;
    }

    double diff_seconds = std::difftime(time_target, time_today);
    return static_cast<int>(std::round(diff_seconds / 86400.0));
}

std::string format_countdown(std::string_view exam_date_iso) {
    int days = get_days_until(exam_date_iso);
    if (days > 1) {
        return std::to_string(days) + " giorni";
    }
    if (days == 1) {
        return "1 giorno";
    }
    if (days == 0) {
        return "OGGI";
    }
    if (days == -1) {
        return "Passato ieri";
    }
    return "Passato da " + std::to_string(-days) + " giorni";
}

bool is_in_current_week(std::string_view date_iso) {
    auto parsed = parse_date_iso(date_iso);
    if (!parsed) {
        return false;
    }

    std::tm tm_now = get_local_tm(std::time(nullptr));

    // tm_wday: 0 = Sunday, 1 = Monday, ..., 6 = Saturday
    int iso_wday = (tm_now.tm_wday == 0) ? 7 : tm_now.tm_wday;
    int days_since_monday = iso_wday - 1;

    std::tm tm_monday = tm_now;
    tm_monday.tm_mday -= days_since_monday;
    tm_monday.tm_hour = 0;
    tm_monday.tm_min = 0;
    tm_monday.tm_sec = 0;
    tm_monday.tm_isdst = -1;
    std::time_t monday_t = std::mktime(&tm_monday);

    std::time_t sunday_t = monday_t + (7 * 86400) - 1;

    std::tm tm_target{};
    tm_target.tm_year = parsed->year - 1900;
    tm_target.tm_mon = parsed->month - 1;
    tm_target.tm_mday = parsed->day;
    tm_target.tm_hour = 12;
    tm_target.tm_isdst = -1;
    std::time_t target_t = std::mktime(&tm_target);

    return target_t != -1 && monday_t != -1 && (target_t >= monday_t && target_t <= sunday_t);
}

} // namespace tracker
