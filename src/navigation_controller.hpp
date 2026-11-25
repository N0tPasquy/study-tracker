#pragma once

#include "model.hpp"

namespace tracker {

/// Manages view state, focus sections, selection indices, and roadmap viewport scrolling.
class NavigationController {
public:
    void clamp_initial_indices(Roadmap& roadmap);

    bool select_block(Roadmap& roadmap, int block_index);
    bool select_segment(Roadmap& roadmap, int segment_index);

    void move_task_selection(const Block& block, int delta);
    void move_resource_selection(const Block& block, int delta);
    void move_roadmap_selection(const Roadmap& roadmap, int delta);

    void jump_roadmap_start() { roadmap_selected_index_ = 0; }
    void jump_roadmap_end(const Roadmap& roadmap);
    void adjust_roadmap_scroll(int total_blocks, int visible_count);

    void toggle_focus_section();

    [[nodiscard]] ViewState view_state() const { return view_state_; }
    void set_view_state(ViewState state) { view_state_ = state; }

    [[nodiscard]] FocusSection focus_section() const { return focus_section_; }
    void set_focus_section(FocusSection section) { focus_section_ = section; }

    [[nodiscard]] int selected_task_index() const { return selected_task_index_; }
    void set_selected_task_index(int idx) { selected_task_index_ = idx; }

    [[nodiscard]] int selected_resource_index() const { return selected_resource_index_; }
    void set_selected_resource_index(int idx) { selected_resource_index_ = idx; }

    [[nodiscard]] int roadmap_selected_index() const { return roadmap_selected_index_; }
    void set_roadmap_selected_index(int idx) { roadmap_selected_index_ = idx; }

    [[nodiscard]] int roadmap_scroll_offset() const { return roadmap_scroll_offset_; }

private:
    ViewState view_state_ = ViewState::Focus;
    FocusSection focus_section_ = FocusSection::Tasks;

    int selected_task_index_ = 0;
    int selected_resource_index_ = 0;
    int roadmap_selected_index_ = 0;
    int roadmap_scroll_offset_ = 0;
};

} // namespace tracker
