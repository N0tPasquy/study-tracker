#include "stats.hpp"
#include "date_utils.hpp"

#include <cstdio>

namespace tracker {

int count_weekly_completed_blocks(const Roadmap& roadmap) {
    int count = 0;
    for (const auto& block : roadmap.blocks) {
        if (block.completed && !block.completed_at.empty() && is_in_current_week(block.completed_at)) {
            count++;
        }
    }
    return count;
}

int count_total_completed_blocks(const Roadmap& roadmap) {
    int count = 0;
    for (const auto& block : roadmap.blocks) {
        if (block.completed) {
            count++;
        }
    }
    return count;
}

std::string format_time_mm_ss(int total_seconds) {
    int minutes = total_seconds / 60;
    int seconds = total_seconds % 60;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", minutes, seconds);
    return std::string(buf);
}

std::string format_duration_hm(int total_seconds) {
    int hours = total_seconds / 3600;
    int minutes = (total_seconds % 3600) / 60;
    if (hours > 0) {
        return std::to_string(hours) + "h " + std::to_string(minutes) + "m";
    }
    return std::to_string(minutes) + "m";
}

ftxui::Color get_phase_color(int phase) {
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
