#include "navigation_controller.hpp"

#include <algorithm>

namespace tracker {

void NavigationController::clamp_initial_indices(Roadmap& roadmap) {
    if (roadmap.current_block_index < 0 ||
        roadmap.current_block_index >= static_cast<int>(roadmap.blocks.size())) {
        roadmap.current_block_index = 0;
    }

    auto& current_block = roadmap.blocks[roadmap.current_block_index];
    if (current_block.current_segment_index < 0 ||
        current_block.current_segment_index >= static_cast<int>(current_block.segments.size())) {
        current_block.current_segment_index = 0;
    }

    roadmap_selected_index_ = roadmap.current_block_index;
}

bool NavigationController::select_block(Roadmap& roadmap, int block_index) {
    if (block_index < 0 || block_index >= static_cast<int>(roadmap.blocks.size())) {
        return false;
    }

    roadmap.current_block_index = block_index;
    selected_task_index_ = 0;
    selected_resource_index_ = 0;

    auto& block = roadmap.blocks[block_index];
    if (block.current_segment_index < 0 ||
        block.current_segment_index >= static_cast<int>(block.segments.size())) {
        block.current_segment_index = 0;
    }

    return true;
}

bool NavigationController::select_segment(Roadmap& roadmap, int segment_index) {
    auto& block = roadmap.blocks[roadmap.current_block_index];
    if (segment_index < 0 || segment_index >= static_cast<int>(block.segments.size())) {
        return false;
    }

    block.current_segment_index = segment_index;
    return true;
}

void NavigationController::move_task_selection(const Block& block, int delta) {
    if (block.tasks.empty()) {
        return;
    }
    int max_idx = static_cast<int>(block.tasks.size()) - 1;
    selected_task_index_ = std::clamp(selected_task_index_ + delta, 0, max_idx);
}

void NavigationController::move_resource_selection(const Block& block, int delta) {
    if (block.resources.empty()) {
        return;
    }
    int max_idx = static_cast<int>(block.resources.size()) - 1;
    selected_resource_index_ = std::clamp(selected_resource_index_ + delta, 0, max_idx);
}

void NavigationController::move_roadmap_selection(const Roadmap& roadmap, int delta) {
    if (roadmap.blocks.empty()) {
        return;
    }
    int max_idx = static_cast<int>(roadmap.blocks.size()) - 1;
    roadmap_selected_index_ = std::clamp(roadmap_selected_index_ + delta, 0, max_idx);
}

void NavigationController::jump_roadmap_end(const Roadmap& roadmap) {
    if (roadmap.blocks.empty()) {
        roadmap_selected_index_ = 0;
        return;
    }
    roadmap_selected_index_ = static_cast<int>(roadmap.blocks.size()) - 1;
}

void NavigationController::adjust_roadmap_scroll(int total_blocks, int visible_count) {
    if (roadmap_selected_index_ < roadmap_scroll_offset_) {
        roadmap_scroll_offset_ = roadmap_selected_index_;
    }
    if (roadmap_selected_index_ >= roadmap_scroll_offset_ + visible_count) {
        roadmap_scroll_offset_ = roadmap_selected_index_ - visible_count + 1;
    }
    int max_offset = std::max(0, total_blocks - visible_count);
    roadmap_scroll_offset_ = std::clamp(roadmap_scroll_offset_, 0, max_offset);
}

void NavigationController::toggle_focus_section() {
    focus_section_ = (focus_section_ == FocusSection::Tasks)
        ? FocusSection::Resources
        : FocusSection::Tasks;
}

} // namespace tracker
