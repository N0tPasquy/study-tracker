#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <nlohmann/json.hpp>
#include <ftxui/screen/color.hpp>

namespace tracker {

struct Segment {
    std::string label;
    std::string type = "study"; // "study" | "break"
    int target_minutes = 60;
    int elapsed_seconds = 0;
    bool done = false;
};

struct Task {
    std::string text;
    bool done = false;
};

struct Resource {
    std::string title;
    std::string url;
    std::string type = "video"; // "video" | "playlist"
    bool watched = false;
};

struct Block {
    int id = 0;
    int phase = 0;
    std::string module;
    std::string title;
    bool completed = false;
    std::string completed_at; // "" oppure "YYYY-MM-DD"
    int current_segment_index = 0;
    std::vector<Segment> segments;
    std::vector<Task> tasks;
    std::vector<Resource> resources;
    std::string last_note;
};

struct Roadmap {
    std::string exam_name;
    std::string exam_date; // "YYYY-MM-DD"
    int weekly_goal_blocks = 3;
    int current_block_index = 0;
    std::vector<Block> blocks;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Segment, label, type, target_minutes, elapsed_seconds, done)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Task, text, done)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Resource, title, url, type, watched)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Block, id, phase, module, title, completed, completed_at, current_segment_index, segments, tasks, resources, last_note)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Roadmap, exam_name, exam_date, weekly_goal_blocks, current_block_index, blocks)

enum class AppMode {
    Normal,
    Insert
};

enum class ViewState {
    Focus,
    Roadmap
};

// Date & Time Helpers

inline std::string get_current_date_iso() {
    std::time_t now = std::time(nullptr);
    std::tm tm_now{};
#if defined(_WIN32)
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                  tm_now.tm_year + 1900,
                  tm_now.tm_mon + 1,
                  tm_now.tm_mday);
    return std::string(buf);
}

inline bool parse_date_iso(const std::string& date_iso, int& year, int& month, int& day) {
    if (date_iso.size() != 10 || date_iso[4] != '-' || date_iso[7] != '-') {
        return false;
    }
    try {
        year = std::stoi(date_iso.substr(0, 4));
        month = std::stoi(date_iso.substr(5, 2));
        day = std::stoi(date_iso.substr(8, 2));
        return true;
    } catch (...) {
        return false;
    }
}

// Calculate days from today to target date (target - today)
inline int get_days_until(const std::string& target_date_iso) {
    int t_year = 0, t_month = 0, t_day = 0;
    if (!parse_date_iso(target_date_iso, t_year, t_month, t_day)) {
        return 0;
    }

    std::time_t now = std::time(nullptr);
    std::tm tm_now{};
#if defined(_WIN32)
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif

    std::tm tm_today{};
    tm_today.tm_year = tm_now.tm_year;
    tm_today.tm_mon = tm_now.tm_mon;
    tm_today.tm_mday = tm_now.tm_mday;
    tm_today.tm_hour = 12;
    tm_today.tm_isdst = -1;

    std::tm tm_target{};
    tm_target.tm_year = t_year - 1900;
    tm_target.tm_mon = t_month - 1;
    tm_target.tm_mday = t_day;
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

inline std::string format_countdown(const std::string& exam_date_iso) {
    int days = get_days_until(exam_date_iso);
    if (days > 1) {
        return std::to_string(days) + " giorni";
    } else if (days == 1) {
        return "1 giorno";
    } else if (days == 0) {
        return "OGGI";
    } else if (days == -1) {
        return "Passato ieri";
    } else {
        return "Passato da " + std::to_string(-days) + " giorni";
    }
}

// Check if a date string falls in the current calendar week (Monday 00:00:00 to Sunday 23:59:59)
inline bool is_in_current_week(const std::string& date_iso) {
    int d_year = 0, d_month = 0, d_day = 0;
    if (!parse_date_iso(date_iso, d_year, d_month, d_day)) {
        return false;
    }

    std::time_t now = std::time(nullptr);
    std::tm tm_now{};
#if defined(_WIN32)
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif

    // tm_wday: 0 = Sunday, 1 = Monday, ..., 6 = Saturday
    int iso_wday = (tm_now.tm_wday == 0) ? 7 : tm_now.tm_wday; // 1 (Mon) .. 7 (Sun)
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
    tm_target.tm_year = d_year - 1900;
    tm_target.tm_mon = d_month - 1;
    tm_target.tm_mday = d_day;
    tm_target.tm_hour = 12;
    tm_target.tm_isdst = -1;
    std::time_t target_t = std::mktime(&tm_target);

    return (target_t >= monday_t && target_t <= sunday_t);
}

inline int count_weekly_completed_blocks(const Roadmap& roadmap) {
    int count = 0;
    for (const auto& block : roadmap.blocks) {
        if (block.completed && !block.completed_at.empty() && is_in_current_week(block.completed_at)) {
            count++;
        }
    }
    return count;
}

inline int count_total_completed_blocks(const Roadmap& roadmap) {
    int count = 0;
    for (const auto& block : roadmap.blocks) {
        if (block.completed) {
            count++;
        }
    }
    return count;
}

inline std::string format_time_mm_ss(int total_seconds) {
    int minutes = total_seconds / 60;
    int seconds = total_seconds % 60;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", minutes, seconds);
    return std::string(buf);
}

inline std::string format_duration_hm(int total_seconds) {
    int hours = total_seconds / 3600;
    int minutes = (total_seconds % 3600) / 60;
    if (hours > 0) {
        return std::to_string(hours) + "h " + std::to_string(minutes) + "m";
    }
    return std::to_string(minutes) + "m";
}

inline ftxui::Color get_phase_color(int phase) {
    switch (phase) {
        case 0: return ftxui::Color::CadetBlue;
        case 1: return ftxui::Color::DeepSkyBlue1;
        case 2: return ftxui::Color::SpringGreen3;
        case 3: return ftxui::Color::Yellow3;
        case 4: return ftxui::Color::MediumPurple2;
        case 5: return ftxui::Color::IndianRed1;
        case 6: return ftxui::Color::Gold1;
        default: return ftxui::Color::White;
    }
}

} // namespace tracker
