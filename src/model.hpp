#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

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
    std::string completed_at; // "" or "YYYY-MM-DD"
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

enum class FocusSection {
    Tasks,
    Resources
};

} // namespace tracker
