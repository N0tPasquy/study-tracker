#include "timer_controller.hpp"

namespace tracker {

bool TimerController::toggle() {
    bool next_state = !running_.load();
    running_.store(next_state);
    return next_state;
}

TimerTickResult TimerController::tick(Roadmap& roadmap) {
    TimerTickResult result{};
    if (!running_.load()) {
        return result;
    }

    if (roadmap.current_block_index < 0 ||
        roadmap.current_block_index >= static_cast<int>(roadmap.blocks.size())) {
        return result;
    }

    auto& block = roadmap.blocks[roadmap.current_block_index];
    if (block.current_segment_index < 0 ||
        block.current_segment_index >= static_cast<int>(block.segments.size())) {
        return result;
    }

    auto& seg = block.segments[block.current_segment_index];
    seg.elapsed_seconds++;
    result.elapsed_incremented = true;
    ticks_since_save_++;

    const int target_seconds = seg.target_minutes * 60;
    if (seg.elapsed_seconds == target_seconds) {
        seg.done = true;
        running_.store(false);
        ticks_since_save_ = 0;
        result.segment_completed = true;
        result.completed_segment_label = seg.label;
        result.autosave_due = true;
    } else if (ticks_since_save_ >= 30) {
        ticks_since_save_ = 0;
        result.autosave_due = true;
    }

    return result;
}

} // namespace tracker
