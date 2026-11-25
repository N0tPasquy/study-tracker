#pragma once

#include "model.hpp"
#include "navigation_controller.hpp"
#include "persistence_controller.hpp"
#include "timer_controller.hpp"
#include "url_opener.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

namespace tracker {

using UrlOpenerFunc = std::function<bool(const std::string&)>;

class UIController {
public:
    UIController(Roadmap& roadmap,
                 const std::filesystem::path& file_path,
                 ftxui::ScreenInteractive& screen,
                 UrlOpenerFunc url_opener = nullptr);

    ftxui::Component CreateComponent();

    void tick_timer();
    void trigger_save();
    void notify_alert(const std::string& msg);
    void dismiss_alert();

    [[nodiscard]] bool is_timer_running() const { return timer_.is_running(); }
    void set_timer_running(bool running) { timer_.set_running(running); }
    void toggle_timer();

    [[nodiscard]] bool is_app_running() const { return app_running_.load(); }
    void stop_app() { app_running_.store(false); }

    [[nodiscard]] bool is_data_saved() const { return persistence_.is_saved(); }
    [[nodiscard]] const std::string& get_alert_message() const { return alert_message_; }

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

    [[nodiscard]] FocusSection get_focus_section() const { return navigation_.focus_section(); }
    void set_focus_section(FocusSection section) { navigation_.set_focus_section(section); }

    [[nodiscard]] int get_selected_task_index() const { return navigation_.selected_task_index(); }
    void set_selected_task_index(int idx) { navigation_.set_selected_task_index(idx); }

    [[nodiscard]] int get_selected_resource_index() const { return navigation_.selected_resource_index(); }
    void set_selected_resource_index(int idx) { navigation_.set_selected_resource_index(idx); }

    [[nodiscard]] int get_note_cursor() const { return note_cursor_; }

    void set_url_opener(UrlOpenerFunc opener) { url_opener_ = std::move(opener); }

private:
    Roadmap& roadmap_;
    ftxui::ScreenInteractive& screen_;

    PersistenceController persistence_;
    TimerController timer_;
    NavigationController navigation_;

    AppMode mode_ = AppMode::Normal;
    UrlOpenerFunc url_opener_;

    std::string note_buffer_;
    int note_cursor_ = 0;
    ftxui::Box note_box_;

    std::atomic<bool> app_running_{true};

    std::string alert_message_;
    int alert_ticks_ = 0;

    char pending_key_ = '\0';
    std::chrono::steady_clock::time_point pending_key_time_;

    bool handle_insert_event(ftxui::Event& event);
    bool handle_normal_event(ftxui::Event& event);
    bool handle_focus_event(const ftxui::Event& event);
    bool handle_roadmap_event(const ftxui::Event& event);

    void sync_note_buffer_from_block();
    void sync_note_buffer_to_block();
};

} // namespace tracker
