#pragma once

#include "model.hpp"
#include <filesystem>
#include <memory>
#include <atomic>
#include <chrono>
#include <string>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>

namespace tracker {

class UIController {
public:
    UIController(Roadmap& roadmap, const std::filesystem::path& file_path, ftxui::ScreenInteractive& screen);

    ftxui::Component CreateComponent();

    void tick_timer();
    void trigger_save();
    void notify_alert(const std::string& msg);
    void dismiss_alert();

    bool is_timer_running() const { return timer_running_.load(); }
    void set_timer_running(bool running) { timer_running_.store(running); }
    void toggle_timer();

    bool is_app_running() const { return app_running_.load(); }
    void stop_app() { app_running_.store(false); }

    void switch_to_block(int block_index);
    void switch_to_segment(int segment_index);
    void enter_insert_mode(bool append);
    void exit_insert_mode();
    void toggle_block_completed();
    void toggle_current_task();
    void toggle_task_at(int task_index);
    void complete_current_segment();
    void reset_current_segment();

private:
    Roadmap& roadmap_;
    std::filesystem::path file_path_;
    ftxui::ScreenInteractive& screen_;

    AppMode mode_ = AppMode::Normal;
    ViewState view_state_ = ViewState::Focus;

    int selected_task_index_ = 0;
    int roadmap_selected_index_ = 0;
    int roadmap_scroll_offset_ = 0;

    std::string note_buffer_;
    int note_cursor_ = 0;
    ftxui::Box note_box_;

    std::atomic<bool> app_running_{true};
    std::atomic<bool> timer_running_{false};
    bool data_saved_ = true;
    int timer_ticks_since_save_ = 0;

    std::string alert_message_;
    int alert_ticks_ = 0;

    char pending_key_ = '\0';
    std::chrono::steady_clock::time_point pending_key_time_;

    ftxui::Element RenderHeader();
    ftxui::Element RenderFocusView();
    ftxui::Element RenderRoadmapView();
    ftxui::Element RenderFooter();

    void sync_note_buffer_from_block();
    void sync_note_buffer_to_block();
};

} // namespace tracker
