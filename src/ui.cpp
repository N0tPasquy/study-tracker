#include "ui.hpp"
#include "date_utils.hpp"
#include "ui_renderer.hpp"

#include <iostream>
#include <stdexcept>
#include <utility>
#include <ftxui/component/component_options.hpp>

namespace tracker {

UIController::UIController(Roadmap& roadmap,
                           const std::filesystem::path& file_path,
                           ftxui::ScreenInteractive& screen,
                           UrlOpenerFunc url_opener)
    : roadmap_(roadmap),
      screen_(screen),
      persistence_(file_path),
      url_opener_(std::move(url_opener)) {
    if (!url_opener_) {
        url_opener_ = open_url_in_browser;
    }

    if (roadmap_.blocks.empty()) {
        throw std::runtime_error("Roadmap non contiene alcun blocco!");
    }

    navigation_.clamp_initial_indices(roadmap_);
    sync_note_buffer_from_block();
}

void UIController::sync_note_buffer_from_block() {
    if (roadmap_.current_block_index >= 0 &&
        roadmap_.current_block_index < static_cast<int>(roadmap_.blocks.size())) {
        note_buffer_ = roadmap_.blocks[roadmap_.current_block_index].last_note;
        note_cursor_ = static_cast<int>(note_buffer_.size());
    }
}

void UIController::sync_note_buffer_to_block() {
    if (roadmap_.current_block_index >= 0 &&
        roadmap_.current_block_index < static_cast<int>(roadmap_.blocks.size())) {
        if (roadmap_.blocks[roadmap_.current_block_index].last_note != note_buffer_) {
            roadmap_.blocks[roadmap_.current_block_index].last_note = note_buffer_;
            persistence_.mark_dirty();
        }
    }
}

void UIController::switch_to_block(int block_index) {
    if (block_index < 0 || block_index >= static_cast<int>(roadmap_.blocks.size())) {
        return;
    }

    timer_.pause();
    sync_note_buffer_to_block();
    if (!navigation_.select_block(roadmap_, block_index)) {
        return;
    }

    sync_note_buffer_from_block();
    dismiss_alert();
    persistence_.mark_dirty();
    trigger_save();
}

void UIController::switch_to_segment(int segment_index) {
    auto& block = roadmap_.blocks[roadmap_.current_block_index];
    if (segment_index < 0 || segment_index >= static_cast<int>(block.segments.size())) {
        return;
    }

    timer_.pause();
    navigation_.select_segment(roadmap_, segment_index);
    dismiss_alert();
    persistence_.mark_dirty();
    trigger_save();
}

void UIController::enter_insert_mode(bool append) {
    mode_ = AppMode::Insert;
    note_cursor_ = append ? static_cast<int>(note_buffer_.size()) : 0;
}

void UIController::exit_insert_mode() {
    mode_ = AppMode::Normal;
    sync_note_buffer_to_block();
    trigger_save();
}

void UIController::toggle_block_completed() {
    auto& block = roadmap_.blocks[roadmap_.current_block_index];
    block.completed = !block.completed;
    block.completed_at = block.completed ? get_current_date_iso() : "";
    persistence_.mark_dirty();
    trigger_save();
}

void UIController::toggle_current_task() {
    toggle_task_at(navigation_.selected_task_index());
}

void UIController::toggle_task_at(int task_index) {
    auto& block = roadmap_.blocks[roadmap_.current_block_index];
    if (task_index >= 0 && task_index < static_cast<int>(block.tasks.size())) {
        block.tasks[task_index].done = !block.tasks[task_index].done;
        persistence_.mark_dirty();
        trigger_save();
    }
}

void UIController::toggle_current_resource() {
    toggle_resource_at(navigation_.selected_resource_index());
}

void UIController::toggle_resource_at(int resource_index) {
    auto& block = roadmap_.blocks[roadmap_.current_block_index];
    if (resource_index >= 0 && resource_index < static_cast<int>(block.resources.size())) {
        block.resources[resource_index].watched = !block.resources[resource_index].watched;
        persistence_.mark_dirty();
        trigger_save();
    }
}

void UIController::open_current_resource() {
    open_resource_at(navigation_.selected_resource_index());
}

void UIController::open_resource_at(int resource_index) {
    auto& block = roadmap_.blocks[roadmap_.current_block_index];
    if (resource_index < 0 || resource_index >= static_cast<int>(block.resources.size())) {
        return;
    }

    const auto& res = block.resources[resource_index];
    if (!is_valid_http_url(res.url)) {
        notify_alert("URL non valido: " + res.url);
        return;
    }

    notify_alert("Apertura nel browser: " + res.title);
    if (url_opener_) {
        if (!url_opener_(res.url)) {
            notify_alert("Impossibile aprire il browser per: " + res.url);
        }
    }
}

void UIController::toggle_current_segment() {
    auto& block = roadmap_.blocks[roadmap_.current_block_index];
    if (block.current_segment_index < 0 ||
        block.current_segment_index >= static_cast<int>(block.segments.size())) {
        return;
    }

    auto& seg = block.segments[block.current_segment_index];
    if (!seg.done) {
        seg.done = true;
        if (block.current_segment_index + 1 < static_cast<int>(block.segments.size())) {
            switch_to_segment(block.current_segment_index + 1);
            return;
        }
    } else {
        seg.done = false;
    }

    persistence_.mark_dirty();
    trigger_save();
}

void UIController::complete_current_segment() {
    toggle_current_segment();
}

void UIController::reset_current_segment() {
    auto& block = roadmap_.blocks[roadmap_.current_block_index];
    if (block.current_segment_index >= 0 &&
        block.current_segment_index < static_cast<int>(block.segments.size())) {
        auto& seg = block.segments[block.current_segment_index];
        seg.elapsed_seconds = 0;
        seg.done = false;
        persistence_.mark_dirty();
        trigger_save();
    }
}

void UIController::toggle_timer() {
    if (timer_.toggle()) {
        dismiss_alert();
    }
    trigger_save();
}

void UIController::tick_timer() {
    if (!alert_message_.empty()) {
        alert_ticks_++;
        if (alert_ticks_ >= 6) {
            dismiss_alert();
        }
    }

    const TimerTickResult result = timer_.tick(roadmap_);
    if (!result.elapsed_incremented) {
        return;
    }

    persistence_.mark_dirty();

    if (result.segment_completed) {
        std::cout << '\a' << std::flush;
        notify_alert("Segmento '" + result.completed_segment_label +
                     "' completato! Premi [l] per avanzare o [t] per overtime.");
    }

    if (result.autosave_due) {
        trigger_save();
    }
}

void UIController::trigger_save() {
    sync_note_buffer_to_block();
    if (!persistence_.save(roadmap_)) {
        notify_alert("⚠ Errore: salvataggio fallito!");
    }
}

void UIController::notify_alert(const std::string& msg) {
    alert_message_ = msg;
    alert_ticks_ = 0;
}

void UIController::dismiss_alert() {
    alert_message_.clear();
    alert_ticks_ = 0;
}

bool UIController::handle_insert_event(ftxui::Event& event) {
    using namespace ftxui;

    if (event == Event::Escape || event == Event::Special({27}) || event == Event::Return) {
        exit_insert_mode();
        return true;
    }
    if (event == Event::Special({19})) { // Ctrl+S
        trigger_save();
        return true;
    }

    if (event.is_mouse() && event.mouse().button == Mouse::Left &&
        event.mouse().motion == Mouse::Pressed) {
        if (!note_box_.Contain(event.mouse().x, event.mouse().y)) {
            exit_insert_mode();
            return true;
        }
    }

    // Pass all other keys to FTXUI Input component
    persistence_.mark_dirty();
    return false;
}

bool UIController::handle_focus_event(const ftxui::Event& event) {
    using namespace ftxui;

    if (event == Event::Tab || event == Event::TabReverse) {
        navigation_.toggle_focus_section();
        return true;
    }

    const auto& block = roadmap_.blocks[roadmap_.current_block_index];

    if (navigation_.focus_section() == FocusSection::Tasks) {
        if (event.character() == "j" || event == Event::ArrowDown) {
            navigation_.move_task_selection(block, 1);
            return true;
        }
        if (event.character() == "k" || event == Event::ArrowUp) {
            navigation_.move_task_selection(block, -1);
            return true;
        }
        if (event.character() == "x" || event.character() == " " || event == Event::Return) {
            toggle_current_task();
            return true;
        }
    } else if (navigation_.focus_section() == FocusSection::Resources) {
        if (event.character() == "j" || event == Event::ArrowDown) {
            navigation_.move_resource_selection(block, 1);
            return true;
        }
        if (event.character() == "k" || event == Event::ArrowUp) {
            navigation_.move_resource_selection(block, -1);
            return true;
        }
        if (event.character() == "x" || event.character() == " ") {
            toggle_current_resource();
            return true;
        }
        if (event == Event::Return || event.character() == "o") {
            open_current_resource();
            return true;
        }
    }

    // Segment Navigation
    if (event.character() == "h" || event.character() == "[") {
        if (block.current_segment_index > 0) {
            switch_to_segment(block.current_segment_index - 1);
        }
        return true;
    }
    if (event.character() == "l" || event.character() == "]") {
        if (block.current_segment_index + 1 < static_cast<int>(block.segments.size())) {
            switch_to_segment(block.current_segment_index + 1);
        }
        return true;
    }

    // Block Navigation
    if (event.character() == "H") {
        if (roadmap_.current_block_index > 0) {
            switch_to_block(roadmap_.current_block_index - 1);
        }
        return true;
    }
    if (event.character() == "L") {
        if (roadmap_.current_block_index + 1 < static_cast<int>(roadmap_.blocks.size())) {
            switch_to_block(roadmap_.current_block_index + 1);
        }
        return true;
    }

    // Timer & Block Controls
    if (event.character() == "t") {
        toggle_timer();
        return true;
    }
    if (event.character() == "c") {
        complete_current_segment();
        return true;
    }
    if (event.character() == "R") {
        reset_current_segment();
        return true;
    }
    if (event.character() == "M") {
        toggle_block_completed();
        return true;
    }

    // Enter Insert Mode
    if (event.character() == "i") {
        enter_insert_mode(false);
        return true;
    }
    if (event.character() == "a") {
        enter_insert_mode(true);
        return true;
    }

    return true;
}

bool UIController::handle_roadmap_event(const ftxui::Event& event) {
    using namespace ftxui;

    if (event == Event::Tab || event == Event::Escape || event == Event::Special({27})) {
        navigation_.set_view_state(ViewState::Focus);
        return true;
    }

    if (event.character() == "j" || event == Event::ArrowDown) {
        navigation_.move_roadmap_selection(roadmap_, 1);
        return true;
    }
    if (event.character() == "k" || event == Event::ArrowUp) {
        navigation_.move_roadmap_selection(roadmap_, -1);
        return true;
    }
    if (event == Event::Special({4})) { // Ctrl+D
        navigation_.move_roadmap_selection(roadmap_, 7);
        return true;
    }
    if (event == Event::Special({21})) { // Ctrl+U
        navigation_.move_roadmap_selection(roadmap_, -7);
        return true;
    }
    if (event.character() == "G") {
        navigation_.jump_roadmap_end(roadmap_);
        return true;
    }
    if (event == Event::Return || event.character() == "l") {
        switch_to_block(navigation_.roadmap_selected_index());
        navigation_.set_view_state(ViewState::Focus);
        return true;
    }

    return true;
}

bool UIController::handle_normal_event(ftxui::Event& event) {
    using namespace ftxui;

    // 2-key sequences ('gg' and 'ZZ')
    if (pending_key_ == 'g') {
        pending_key_ = '\0';
        if (event.character() == "g") {
            if (navigation_.view_state() == ViewState::Roadmap) {
                navigation_.jump_roadmap_start();
            }
            return true;
        }
    } else if (pending_key_ == 'Z') {
        pending_key_ = '\0';
        if (event.character() == "Z") {
            trigger_save();
            stop_app();
            screen_.ExitLoopClosure()();
            return true;
        }
    }

    // Mouse click on note box in Normal mode enters Insert mode
    if (event.is_mouse() && event.mouse().button == Mouse::Left &&
        event.mouse().motion == Mouse::Pressed) {
        if (note_box_.Contain(event.mouse().x, event.mouse().y)) {
            enter_insert_mode(false);
            return true;
        }
    }

    // Global Shortcuts
    if (event.character() == "r") {
        if (navigation_.view_state() == ViewState::Focus) {
            navigation_.set_view_state(ViewState::Roadmap);
            navigation_.set_roadmap_selected_index(roadmap_.current_block_index);
        } else {
            navigation_.set_view_state(ViewState::Focus);
        }
        return true;
    }

    if (event.character() == "w" || event == Event::Special({19})) { // 'w' or Ctrl+S
        trigger_save();
        return true;
    }

    if (event == Event::Special({17})) { // Ctrl+Q
        trigger_save();
        stop_app();
        screen_.ExitLoopClosure()();
        return true;
    }

    if (event.character() == "q") {
        if (navigation_.view_state() == ViewState::Roadmap) {
            navigation_.set_view_state(ViewState::Focus);
            return true;
        }
        trigger_save();
        stop_app();
        screen_.ExitLoopClosure()();
        return true;
    }

    if (event.character() == "g") {
        pending_key_ = 'g';
        pending_key_time_ = std::chrono::steady_clock::now();
        return true;
    }

    if (event.character() == "Z") {
        pending_key_ = 'Z';
        pending_key_time_ = std::chrono::steady_clock::now();
        return true;
    }

    if (navigation_.view_state() == ViewState::Focus) {
        return handle_focus_event(event);
    }
    return handle_roadmap_event(event);
}

ftxui::Component UIController::CreateComponent() {
    using namespace ftxui;

    InputOption input_option;
    input_option.cursor_position = &note_cursor_;

    auto input_component = Input(&note_buffer_, "Scrivi qui a che punto sei o i dubbi...", input_option);

    auto main_renderer = Renderer(input_component, [&] {
        UIRenderContext ctx{
            roadmap_,
            navigation_,
            mode_,
            note_buffer_,
            note_cursor_,
            note_box_,
            timer_.is_running(),
            persistence_.is_saved(),
            alert_message_
        };

        Element view_content = (navigation_.view_state() == ViewState::Focus)
            ? render_focus_view(ctx)
            : render_roadmap_view(roadmap_, navigation_);

        return vbox({
            render_header(roadmap_),
            view_content | flex,
            render_footer(ctx),
        });
    });

    return CatchEvent(main_renderer, [&](Event event) -> bool {
        if (pending_key_ != '\0') {
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - pending_key_time_).count();
            if (elapsed > 1000) {
                pending_key_ = '\0';
            }
        }

        if (mode_ == AppMode::Insert) {
            return handle_insert_event(event);
        }
        return handle_normal_event(event);
    });
}

} // namespace tracker
