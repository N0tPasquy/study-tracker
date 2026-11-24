#pragma once

#include "model.hpp"
#include "url_opener.hpp"
#include <filesystem>
#include <memory>
#include <atomic>
#include <chrono>
#include <string>
#include <functional>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>

namespace tracker {

enum class FocusSection {
    Tasks,
    Resources
};

using UrlOpenerFunc = std::function<bool(const std::string&)>;

class UIController {
public:
    UIController(Roadmap& roadmap, const std::filesystem::path& file_path, ftxui::ScreenInteractive& screen, UrlOpenerFunc url_opener = nullptr);

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
    void toggle_current_resource();
    void toggle_resource_at(int resource_index);
    void open_current_resource();
    void open_resource_at(int resource_index);
    void toggle_current_segment();
    void complete_current_segment();
    void reset_current_segment();

    FocusSection get_focus_section() const { return focus_section_; }
    void set_focus_section(FocusSection section) { focus_section_ = section; }

    int get_selected_task_index() const { return selected_task_index_; }
    void set_selected_task_index(int idx) { selected_task_index_ = idx; }

    int get_selected_resource_index() const { return selected_resource_index_; }
    void set_selected_resource_index(int idx) { selected_resource_index_ = idx; }

    int get_note_cursor() const { return note_cursor_; }

    void set_url_opener(UrlOpenerFunc opener) { url_opener_ = std::move(opener); }

private:
    Roadmap& roadmap_;
    std::filesystem::path file_path_;
    ftxui::ScreenInteractive& screen_;

    AppMode mode_ = AppMode::Normal;
    ViewState view_state_ = ViewState::Focus;
    FocusSection focus_section_ = FocusSection::Tasks;

    int selected_task_index_ = 0;
    int selected_resource_index_ = 0;
    int roadmap_selected_index_ = 0;
    int roadmap_scroll_offset_ = 0;

    UrlOpenerFunc url_opener_;

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
