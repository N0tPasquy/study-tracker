#include "ui.hpp"
#include "storage.hpp"

#include <algorithm>
#include <iostream>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

namespace tracker {

using namespace ftxui;

UIController::UIController(Roadmap& roadmap, const std::filesystem::path& file_path, ScreenInteractive& screen)
    : roadmap_(roadmap), file_path_(file_path), screen_(screen) {
    if (roadmap_.blocks.empty()) {
        throw std::runtime_error("Roadmap non contiene alcun blocco!");
    }

    if (roadmap_.current_block_index < 0 || roadmap_.current_block_index >= static_cast<int>(roadmap_.blocks.size())) {
        roadmap_.current_block_index = 0;
    }

    auto& current_block = roadmap_.blocks[roadmap_.current_block_index];
    if (current_block.current_segment_index < 0 ||
        current_block.current_segment_index >= static_cast<int>(current_block.segments.size())) {
        current_block.current_segment_index = 0;
    }

    roadmap_selected_index_ = roadmap_.current_block_index;
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
            data_saved_ = false;
        }
    }
}

void UIController::switch_to_block(int block_index) {
    if (block_index < 0 || block_index >= static_cast<int>(roadmap_.blocks.size())) {
        return;
    }

    if (timer_running_.load()) {
        timer_running_.store(false);
    }

    sync_note_buffer_to_block();
    roadmap_.current_block_index = block_index;
    selected_task_index_ = 0;

    auto& block = roadmap_.blocks[block_index];
    if (block.current_segment_index < 0 ||
        block.current_segment_index >= static_cast<int>(block.segments.size())) {
        block.current_segment_index = 0;
    }

    sync_note_buffer_from_block();
    dismiss_alert();
    data_saved_ = false;
    trigger_save();
}

void UIController::switch_to_segment(int segment_index) {
    auto& block = roadmap_.blocks[roadmap_.current_block_index];
    if (segment_index < 0 || segment_index >= static_cast<int>(block.segments.size())) {
        return;
    }

    if (timer_running_.load()) {
        timer_running_.store(false);
    }

    block.current_segment_index = segment_index;
    dismiss_alert();
    data_saved_ = false;
    trigger_save();
}

void UIController::enter_insert_mode(bool append) {
    mode_ = AppMode::Insert;
    if (append) {
        note_cursor_ = static_cast<int>(note_buffer_.size());
    } else {
        note_cursor_ = 0;
    }
}

void UIController::exit_insert_mode() {
    mode_ = AppMode::Normal;
    sync_note_buffer_to_block();
    trigger_save();
}

void UIController::toggle_block_completed() {
    auto& block = roadmap_.blocks[roadmap_.current_block_index];
    block.completed = !block.completed;
    if (block.completed) {
        block.completed_at = get_current_date_iso();
    } else {
        block.completed_at = "";
    }
    data_saved_ = false;
    trigger_save();
}

void UIController::toggle_current_task() {
    toggle_task_at(selected_task_index_);
}

void UIController::toggle_task_at(int task_index) {
    auto& block = roadmap_.blocks[roadmap_.current_block_index];
    if (task_index >= 0 && task_index < static_cast<int>(block.tasks.size())) {
        block.tasks[task_index].done = !block.tasks[task_index].done;
        data_saved_ = false;
        trigger_save();
    }
}

void UIController::complete_current_segment() {
    auto& block = roadmap_.blocks[roadmap_.current_block_index];
    if (block.current_segment_index >= 0 &&
        block.current_segment_index < static_cast<int>(block.segments.size())) {
        block.segments[block.current_segment_index].done = true;
        if (block.current_segment_index + 1 < static_cast<int>(block.segments.size())) {
            switch_to_segment(block.current_segment_index + 1);
        } else {
            data_saved_ = false;
            trigger_save();
        }
    }
}

void UIController::reset_current_segment() {
    auto& block = roadmap_.blocks[roadmap_.current_block_index];
    if (block.current_segment_index >= 0 &&
        block.current_segment_index < static_cast<int>(block.segments.size())) {
        block.segments[block.current_segment_index].elapsed_seconds = 0;
        data_saved_ = false;
        trigger_save();
    }
}

void UIController::toggle_timer() {
    bool next_state = !timer_running_.load();
    timer_running_.store(next_state);
    if (next_state) {
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

    if (!timer_running_.load()) {
        return;
    }

    auto& block = roadmap_.blocks[roadmap_.current_block_index];
    if (block.current_segment_index >= 0 &&
        block.current_segment_index < static_cast<int>(block.segments.size())) {
        auto& seg = block.segments[block.current_segment_index];
        seg.elapsed_seconds++;
        data_saved_ = false;
        timer_ticks_since_save_++;

        int target_seconds = seg.target_minutes * 60;
        if (seg.elapsed_seconds == target_seconds) {
            seg.done = true;
            timer_running_.store(false);
            std::cout << '\a' << std::flush;
            notify_alert("Segmento '" + seg.label + "' completato! Premi [l] per avanzare o [t] per overtime.");
            trigger_save();
        } else if (timer_ticks_since_save_ >= 30) {
            trigger_save();
            timer_ticks_since_save_ = 0;
        }
    }
}

void UIController::trigger_save() {
    sync_note_buffer_to_block();
    save_roadmap(roadmap_, file_path_);
    data_saved_ = true;
}

void UIController::notify_alert(const std::string& msg) {
    alert_message_ = msg;
    alert_ticks_ = 0;
}

void UIController::dismiss_alert() {
    alert_message_.clear();
    alert_ticks_ = 0;
}

// ----------------- RENDERING -----------------

Element UIController::RenderHeader() {
    int weekly_done = count_weekly_completed_blocks(roadmap_);
    int total_done = count_total_completed_blocks(roadmap_);
    int total_blocks = static_cast<int>(roadmap_.blocks.size());
    float global_ratio = (total_blocks > 0) ? (static_cast<float>(total_done) / total_blocks) : 0.0f;
    int percent = static_cast<int>(global_ratio * 100.0f);

    Color weekly_color = (weekly_done >= roadmap_.weekly_goal_blocks) ? Color(Color::GreenLight) : Color(Color::Yellow);

    return vbox({
        hbox({
            text(" " + roadmap_.exam_name + " ") | bold | color(Color::White) | bgcolor(Color::BlueLight),
            separator(),
            text(" Esame: " + roadmap_.exam_date + " (" + format_countdown(roadmap_.exam_date) + ") ") | color(Color::CyanLight),
            separator(),
            text(" Obiettivo Settimana: " + std::to_string(weekly_done) + "/" +
                 std::to_string(roadmap_.weekly_goal_blocks) + " ") | bold | color(weekly_color),
            filler(),
            text(" Progresso: " + std::to_string(total_done) + "/" + std::to_string(total_blocks) +
                 " (" + std::to_string(percent) + "%) ") | bold | color(Color::White),
            gauge(global_ratio) | size(WIDTH, EQUAL, 18) | color(Color::Green),
        }),
        separatorLight(),
    });
}

Element UIController::RenderFocusView() {
    auto& block = roadmap_.blocks[roadmap_.current_block_index];

    // 1. Block Header
    auto phase_badge = text(" " + block.module + " ") | bold |
                       bgcolor(get_phase_color(block.phase)) | color(Color::Black);
    auto block_title = text(" [#" + std::to_string(block.id) + "] " + block.title) | bold | color(Color::White);

    Element status_badge;
    if (block.completed) {
        status_badge = text(" [COMPLETATO" + (block.completed_at.empty() ? "" : ": " + block.completed_at) + "] ") |
                       bold | color(Color::GreenLight);
    } else {
        status_badge = text(" [IN CORSO] ") | bold | color(Color::YellowLight);
    }

    auto block_header = hbox({
        phase_badge,
        text(" "),
        block_title,
        filler(),
        status_badge,
        text(" [M: Toggle] ") | dim,
    });

    // 2. Segments & Timer Panel
    Elements seg_elements;
    int study_elapsed = 0, study_target = 0;
    int break_elapsed = 0, break_target = 0;

    for (int i = 0; i < static_cast<int>(block.segments.size()); ++i) {
        const auto& seg = block.segments[i];
        bool is_active = (i == block.current_segment_index);

        if (seg.type == "study") {
            study_elapsed += seg.elapsed_seconds;
            study_target += seg.target_minutes * 60;
        } else {
            break_elapsed += seg.elapsed_seconds;
            break_target += seg.target_minutes * 60;
        }

        std::string mark = seg.done ? "[x] " : (is_active ? "[>] " : "[ ] ");
        std::string time_str = format_time_mm_ss(seg.elapsed_seconds) + "/" + format_time_mm_ss(seg.target_minutes * 60);

        Color type_col = (seg.type == "study") ? Color::DeepSkyBlue1 : Color::DarkOrange;
        auto seg_text = text(mark + seg.label + " (" + time_str + ")") | color(type_col);

        if (is_active) {
            seg_elements.push_back(hbox({
                seg_text | bold | inverted,
            }));
        } else {
            seg_elements.push_back(seg_text);
        }

        if (i < static_cast<int>(block.segments.size()) - 1) {
            seg_elements.push_back(text("  │  ") | dim);
        }
    }

    auto segments_row = hbox(std::move(seg_elements));

    // Active Segment Big Timer
    Element timer_display;
    if (block.current_segment_index >= 0 &&
        block.current_segment_index < static_cast<int>(block.segments.size())) {
        const auto& cur_seg = block.segments[block.current_segment_index];
        int target_sec = cur_seg.target_minutes * 60;
        float seg_ratio = (target_sec > 0) ? (static_cast<float>(cur_seg.elapsed_seconds) / target_sec) : 0.0f;
        if (seg_ratio > 1.0f) seg_ratio = 1.0f;

        Color timer_col = cur_seg.done ? Color(Color::GreenLight)
                                       : (timer_running_.load() ? Color(Color::CyanLight) : Color(Color::GrayLight));
        Color seg_badge_bg = (cur_seg.type == "study") ? Color(Color::Blue) : Color(Color::DarkOrange);

        std::string overtime_str;
        if (cur_seg.elapsed_seconds > target_sec) {
            int extra = cur_seg.elapsed_seconds - target_sec;
            overtime_str = " (+" + format_duration_hm(extra) + " Overtime)";
        }

        std::string status_label = timer_running_.load() ? "TIMER IN CORSO" : "IN PAUSA";

        timer_display = vbox({
            hbox({
                text("  " + cur_seg.label + "  ") | bold | color(Color::White) | bgcolor(seg_badge_bg),
                text("  " + format_time_mm_ss(cur_seg.elapsed_seconds) + " / " + format_time_mm_ss(target_sec)) | bold | color(timer_col),
                text(overtime_str) | bold | color(Color::YellowLight),
                text("  [" + status_label + "]") | (timer_running_.load() ? color(Color::Green) : dim),
            }),
            separatorEmpty(),
            gauge(seg_ratio) | color(cur_seg.done ? Color::Green : Color::Cyan),
            separatorEmpty(),
            hbox({
                text("  Comandi: [t] " + std::string(timer_running_.load() ? "Pausa" : "Avvia") +
                     "  [h/l] Seg Prec/Succ  [c] Completa Seg  [R] Reset Seg") | dim,
            }),
        });
    }

    Element alert_box = text("");
    if (!alert_message_.empty()) {
        alert_box = vbox({
            separatorEmpty(),
            hbox({
                filler(),
                text("  🔔 " + alert_message_ + "  ") | bold | bgcolor(Color::Gold1) | color(Color::Black),
                filler(),
            }),
        });
    }

    auto time_summary = hbox({
        text("  Tempo Blocco — Studio: " + format_duration_hm(study_elapsed) + " / " + format_duration_hm(study_target) +
             "  │  Pause: " + format_duration_hm(break_elapsed) + " / " + format_duration_hm(break_target)) | dim,
    });

    auto timer_panel = window(
        text(" Segmenti & Timer ") | bold,
        vbox({
            segments_row,
            separatorLight(),
            timer_display,
            alert_box,
            separatorLight(),
            time_summary,
        })
    );

    // 3. Tasks Checklist
    int done_tasks = 0;
    int total_tasks = static_cast<int>(block.tasks.size());
    for (const auto& t : block.tasks) {
        if (t.done) done_tasks++;
    }

    Elements task_elements;
    for (int i = 0; i < total_tasks; ++i) {
        const auto& t = block.tasks[i];
        bool is_selected = (i == selected_task_index_);

        std::string mark = t.done ? "[✔] " : "[ ] ";
        std::string pointer = is_selected ? "▶ " : "  ";

        auto item_text = text(pointer + mark + t.text);
        if (t.done) {
            item_text = item_text | color(Color::GreenLight);
        }

        if (is_selected) {
            task_elements.push_back(item_text | bold | bgcolor(Color::NavyBlue) | color(Color::White));
        } else {
            task_elements.push_back(item_text);
        }
    }

    if (task_elements.empty()) {
        task_elements.push_back(text("  (Nessun task per questo blocco)") | dim);
    }

    auto checklist_panel = window(
        text(" Checklist Tasks (" + std::to_string(done_tasks) + "/" + std::to_string(total_tasks) + ") [j/k: Muovi, x/Space: Spunta] ") | bold,
        vbox(std::move(task_elements))
    );

    // 4. Last Note Box
    Element note_content;
    if (mode_ == AppMode::Insert) {
        note_content = vbox({
            text("  " + (note_buffer_.empty() ? " " : note_buffer_)) | color(Color::White),
            text("  [MODALITÀ INSERT: Scrivi liberamente. Premi Esc o Enter per salvare] ") | color(Color::YellowLight) | dim,
        });
    } else {
        if (note_buffer_.empty()) {
            note_content = text("  (Nessuna nota. Premi 'i' o 'a' per scrivere dove ti sei fermato...)") | dim;
        } else {
            note_content = text("  " + note_buffer_) | color(Color::White);
        }
    }

    auto note_panel = window(
        text(" Ultimo Punto / Dubbi (last_note) [i/a: Modifica, Esc: Salva] ") | bold,
        note_content
    ) | reflect(note_box_);

    if (mode_ == AppMode::Insert) {
        note_panel = note_panel | color(Color::Gold1);
    }

    return vbox({
        block_header,
        separatorEmpty(),
        timer_panel,
        separatorEmpty(),
        checklist_panel,
        separatorEmpty(),
        note_panel,
    });
}

Element UIController::RenderRoadmapView() {
    int total_blocks = static_cast<int>(roadmap_.blocks.size());
    const int visible_count = 14;

    // Viewport adjustment
    if (roadmap_selected_index_ < roadmap_scroll_offset_) {
        roadmap_scroll_offset_ = roadmap_selected_index_;
    }
    if (roadmap_selected_index_ >= roadmap_scroll_offset_ + visible_count) {
        roadmap_scroll_offset_ = roadmap_selected_index_ - visible_count + 1;
    }
    if (roadmap_scroll_offset_ < 0) roadmap_scroll_offset_ = 0;
    if (roadmap_scroll_offset_ > std::max(0, total_blocks - visible_count)) {
        roadmap_scroll_offset_ = std::max(0, total_blocks - visible_count);
    }

    Elements rows;

    if (roadmap_scroll_offset_ > 0) {
        rows.push_back(text("  ▲ ... altri " + std::to_string(roadmap_scroll_offset_) + " blocchi sopra ...") | dim | center);
    }

    int end_idx = std::min(total_blocks, roadmap_scroll_offset_ + visible_count);
    for (int i = roadmap_scroll_offset_; i < end_idx; ++i) {
        const auto& b = roadmap_.blocks[i];
        bool is_selected = (i == roadmap_selected_index_);
        bool is_current = (i == roadmap_.current_block_index);

        std::string pointer = is_selected ? "▶ " : "  ";
        std::string status_mark = b.completed ? "[✔]" : (is_current ? "[>]" : "[ ]");

        int done_tasks = 0;
        for (const auto& t : b.tasks) {
            if (t.done) done_tasks++;
        }
        std::string tasks_stat = "(" + std::to_string(done_tasks) + "/" + std::to_string(b.tasks.size()) + ")";

        char id_buf[16];
        std::snprintf(id_buf, sizeof(id_buf), "[#%02d]", b.id);

        auto phase_tag = text(" F" + std::to_string(b.phase) + " ") | bold |
                         bgcolor(get_phase_color(b.phase)) | color(Color::Black);

        auto row_elem = hbox({
            text(pointer) | bold | color(Color::Yellow),
            text(status_mark + " ") | (b.completed ? color(Color::GreenLight) : (is_current ? color(Color::CyanLight) : dim)),
            text(std::string(id_buf) + " ") | bold | color(Color::White),
            phase_tag,
            text(" " + b.title) | (b.completed ? color(Color::GrayLight) : color(Color::White)),
            filler(),
            text(tasks_stat + " ") | dim,
            b.completed ? text(b.completed_at + " ") | color(Color::Green) : text("          "),
        });

        if (is_selected) {
            rows.push_back(row_elem | bold | bgcolor(Color::NavyBlue) | color(Color::White));
        } else if (is_current) {
            rows.push_back(row_elem | bgcolor(Color::DarkBlue));
        } else {
            rows.push_back(row_elem);
        }
    }

    if (end_idx < total_blocks) {
        rows.push_back(text("  ▼ ... altri " + std::to_string(total_blocks - end_idx) + " blocchi sotto ...") | dim | center);
    }

    auto list_box = window(
        text(" ROADMAP COMPLETA (" + std::to_string(total_blocks) + " BLOCCHI) ") | bold,
        vbox(std::move(rows))
    );

    return vbox({
        list_box,
        separatorEmpty(),
        hbox({
            filler(),
            text(" Scorri: [j/k] o [↓/↑]  │  Mezza Pagina: [Ctrl+d/u]  │  Inizio/Fine: [gg/G]  │  Seleziona: [Enter/l]  │  Chiudi: [Esc/q] ") | dim,
            filler(),
        }),
    });
}

Element UIController::RenderFooter() {
    Element mode_badge;
    if (mode_ == AppMode::Normal) {
        mode_badge = text(" NORMAL ") | bold | bgcolor(Color::CadetBlue) | color(Color::Black);
    } else {
        mode_badge = text(" INSERT ") | bold | bgcolor(Color::Gold1) | color(Color::Black);
    }

    Element view_badge;
    if (view_state_ == ViewState::Focus) {
        view_badge = text(" FOCUS ") | bold | bgcolor(Color::RoyalBlue1) | color(Color::White);
    } else {
        view_badge = text(" ROADMAP ") | bold | bgcolor(Color::Purple) | color(Color::White);
    }

    Element legend;
    if (mode_ == AppMode::Insert) {
        legend = text(" [Esc/Enter] Salva ed Esci  [Ctrl+S] Salva ") | color(Color::YellowLight);
    } else {
        if (view_state_ == ViewState::Focus) {
            legend = text(" [t] Timer  [h/l] Seg  [H/L] Blocco  [j/k] Tasks  [x] Check  [M] Done  [i/a] Nota  [r] Roadmap  [ZZ/q] Esci ") | dim;
        } else {
            legend = text(" [j/k] Muovi  [Enter/l] Seleziona  [Ctrl+d/u] Pagina  [gg/G] Cima/Fondo  [Esc/q] Indietro ") | dim;
        }
    }

    Element timer_status;
    if (timer_running_.load()) {
        timer_status = text(" TIMER ON ") | bold | bgcolor(Color::GreenLight) | color(Color::Black);
    } else {
        timer_status = text(" PAUSA ") | dim | bgcolor(Color::GrayDark) | color(Color::White);
    }

    Element save_status;
    if (data_saved_) {
        save_status = text(" [Salvato] ") | color(Color::GreenLight);
    } else {
        save_status = text(" [Modificato] ") | bold | color(Color::YellowLight);
    }

    return vbox({
        separatorLight(),
        hbox({
            mode_badge,
            text(" "),
            view_badge,
            text(" "),
            legend,
            filler(),
            timer_status,
            text(" "),
            save_status,
        }),
    });
}

Component UIController::CreateComponent() {
    InputOption input_option;
    input_option.cursor_position = &note_cursor_;

    auto input_component = Input(&note_buffer_, "Scrivi qui a che punto sei o i dubbi...", input_option);

    auto main_renderer = Renderer(input_component, [&] {
        Element view_content;
        if (view_state_ == ViewState::Focus) {
            view_content = RenderFocusView();
        } else {
            view_content = RenderRoadmapView();
        }

        return vbox({
            RenderHeader(),
            view_content | flex,
            RenderFooter(),
        });
    });

    auto root = CatchEvent(main_renderer, [&](Event event) -> bool {
        // Handle pending 2-key sequence expiration (> 1000ms)
        if (pending_key_ != '\0') {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - pending_key_time_).count();
            if (elapsed > 1000) {
                pending_key_ = '\0';
            }
        }

        // --- INSERT MODE HANDLING ---
        if (mode_ == AppMode::Insert) {
            if (event == Event::Escape || event == Event::Special({27})) {
                exit_insert_mode();
                return true;
            }
            if (event == Event::Return) {
                // Opzione A: Enter salva ed esce in Normal
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

            // In Insert mode, pass all other keys to Input component
            data_saved_ = false;
            return false;
        }

        // --- NORMAL MODE HANDLING ---

        // Handle 2-key sequences ('gg' and 'ZZ')
        if (pending_key_ == 'g') {
            if (event.character() == "g") {
                if (view_state_ == ViewState::Roadmap) {
                    roadmap_selected_index_ = 0;
                }
                pending_key_ = '\0';
                return true;
            }
            pending_key_ = '\0';
            // Fall through to process current key normally
        } else if (pending_key_ == 'Z') {
            if (event.character() == "Z") {
                trigger_save();
                stop_app();
                screen_.ExitLoopClosure()();
                pending_key_ = '\0';
                return true;
            }
            pending_key_ = '\0';
            // Fall through to process current key normally
        }

        // Mouse click on note box in Normal mode enters Insert mode
        if (event.is_mouse() && event.mouse().button == Mouse::Left &&
            event.mouse().motion == Mouse::Pressed) {
            if (note_box_.Contain(event.mouse().x, event.mouse().y)) {
                enter_insert_mode(false);
                return true;
            }
        }

        // 1. Global Shortcuts (Both Focus and Roadmap)
        if (event == Event::Tab || event.character() == "r") {
            if (view_state_ == ViewState::Focus) {
                view_state_ = ViewState::Roadmap;
                roadmap_selected_index_ = roadmap_.current_block_index;
            } else {
                view_state_ = ViewState::Focus;
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
            if (view_state_ == ViewState::Roadmap) {
                view_state_ = ViewState::Focus;
                return true;
            } else {
                trigger_save();
                stop_app();
                screen_.ExitLoopClosure()();
                return true;
            }
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

        // 2. View-specific Shortcuts

        if (view_state_ == ViewState::Focus) {
            // Task Checklist Navigation
            if (event.character() == "j" || event == Event::ArrowDown) {
                auto& block = roadmap_.blocks[roadmap_.current_block_index];
                if (!block.tasks.empty()) {
                    selected_task_index_ = std::min(selected_task_index_ + 1, static_cast<int>(block.tasks.size()) - 1);
                }
                return true;
            }

            if (event.character() == "k" || event == Event::ArrowUp) {
                selected_task_index_ = std::max(selected_task_index_ - 1, 0);
                return true;
            }

            if (event.character() == "x" || event.character() == " " || event == Event::Return) {
                toggle_current_task();
                return true;
            }

            // Segment Navigation
            if (event.character() == "h" || event.character() == "[") {
                auto& block = roadmap_.blocks[roadmap_.current_block_index];
                if (block.current_segment_index > 0) {
                    switch_to_segment(block.current_segment_index - 1);
                }
                return true;
            }

            if (event.character() == "l" || event.character() == "]") {
                auto& block = roadmap_.blocks[roadmap_.current_block_index];
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

            // Timer Controls
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

            // Block Completion Toggle
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
        } else if (view_state_ == ViewState::Roadmap) {
            // Roadmap Navigation
            if (event.character() == "j" || event == Event::ArrowDown) {
                roadmap_selected_index_ = std::min(roadmap_selected_index_ + 1, static_cast<int>(roadmap_.blocks.size()) - 1);
                return true;
            }

            if (event.character() == "k" || event == Event::ArrowUp) {
                roadmap_selected_index_ = std::max(roadmap_selected_index_ - 1, 0);
                return true;
            }

            // Half page scroll (Ctrl+D and Ctrl+U)
            if (event == Event::Special({4})) { // Ctrl+D
                roadmap_selected_index_ = std::min(roadmap_selected_index_ + 7, static_cast<int>(roadmap_.blocks.size()) - 1);
                return true;
            }

            if (event == Event::Special({21})) { // Ctrl+U
                roadmap_selected_index_ = std::max(roadmap_selected_index_ - 7, 0);
                return true;
            }

            if (event.character() == "G") {
                roadmap_selected_index_ = static_cast<int>(roadmap_.blocks.size()) - 1;
                return true;
            }

            // Select and return
            if (event == Event::Return || event.character() == "l") {
                switch_to_block(roadmap_selected_index_);
                view_state_ = ViewState::Focus;
                return true;
            }

            // Cancel and return without switching
            if (event == Event::Escape || event == Event::Special({27})) {
                view_state_ = ViewState::Focus;
                return true;
            }
        }

        return false;
    });

    return root;
}

} // namespace tracker
